require 'logger'
require 'thread'

module IRPanel
  class App
    def self.field(name, x, y)
      @fields ||= Hash.new
      raise(ArgumentError, "Field #{name} already defined")\
        if @fields.has_key? name
      @fields[name] = Field.new(name, x, y)
    end

    def self.thread(name, &block)
      @threads ||= Hash.new
      raise(ArgumentError, "Thread #{name} already defined")\
        if @threads.has_key? name
      @threads[name] = block
    end

    def self.context(name, &block)
      @codes ||= Hash.new
      raise(ArgumentError, "Contect #{name} already defined")\
        if @codes.has_key? name
      @context = name
      @codes[@context] = Hash.new
      instance_eval(&block)
    end

    def self.key(name, &block)
      raise(ArgumentError, "Key #{name} defined outside of any context")\
        unless @context
      raise(ArgumentError, "Key #{name} already defined in #{@context}")\
        if @codes[@context].has_key? name
      @codes[@context][name] = block
    end

    def self.fields;        @fields;  end
    def self.threads;       @threads; end
    def self.codes;         @codes;   end
    def self.last_context;  @context; end

    def initialize(socket, config = {})
      @fields = self.class.fields
      @threads = self.class.threads
      @codes = self.class.codes
      @context = self.class.last_context

      @socket = socket
      @config = {
        :keys => 'keys.csv',
        :brightness => 40,
      }.merge!(config)

      @logger = @config[:logger] || Logger.new(STDERR)
      @logger.level = @config[:loglevel] || Logger::DEBUG

      exit(1) unless read_keys(@config[:keys])

      @mutex = Mutex.new
      @rpipe, @wpipe = IO.pipe

      send_cmd("c\n")
      send_cmd("d:#{@config[:brightness]}\n")

      setup
    end

    def setup; end

    def log(level, msg)
      @logger.send(level, msg)
    end

    def send_cmd(str)
      @wpipe.write(str)
    end

    def update(name, text)
      raise(ArgumentError, "No such field #{name}")\
        unless @fields.has_key? name
      if cmd = @fields[name].set(text)
        @mutex.synchronize { @wpipe.write(cmd) }
      end
      @fields[name].data
    end

    def str_clean(str)
      str.encode(Encoding::ASCII, :invalid => :replace,\
                 :undef => :replace, :replace => '')
    end

    def read_keys(file)
      keys = Hash.new

      begin
        File.open(file, 'r') do |fd|
          fd.each_line do |line|
            line.chop!
            key, addr, cmd = line.split(',')
            key.gsub!(/"/, '')
            keys[addr + ':' + cmd] = key
          end
        end
      rescue SystemCallError => e
        log :error, "Reading keys file - #{e}"
        return false
      end

      @keys = keys
    end

    def run!
      trap('SIGTERM') do
        @socket.close
        exit(0)
      end

      trap('SIGINT') do
        @socket.close
        exit(0)
      end

      if @threads
        @threads.each_value do |b|
          Thread.new { instance_eval(&b) }
        end
      end

      while true
        rfds, _, _ = IO.select([@socket, @rpipe], [])
        rfds.each do |fd|
          case fd
          when @socket
            pkt = @socket.readline.chop
            log :debug, "PKT  IN: #{pkt.chop}"
            if pkt.slice(0, 2) == 'ir'
              code = pkt.slice(3, 7)
              if @keys.has_key? code
                key = @keys[code]
                if @context && (@codes[@context].has_key? key)
                  instance_eval(&@codes[@context][key])
                else
                  log :debug, "No code defined for key - #{key}"
                end
              else
                log :debug, "No key defined for code - #{code}"
              end
            else
              log :warn, "Unknown packet - #{pkt}"
            end
          when @rpipe
            IO.select([], [@socket])
            line = @rpipe.readline
            log :debug, "PKT OUT: #{line.chop}"
            @socket.write(line)
            @socket.flush
            res = @socket.readline
            log :debug, "PKT RES: #{res.chop}"
          end
        end
      end
    end
  end
end

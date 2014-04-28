require 'optparse'
require 'socket'

module IRPanel
  module Cli
    def self.run!(app, root, &block)
      @banner = "Usage: #{$0} [OPTIONS] SOCKET|HOST:PORT"
      @config = {
        :keys => File.join(File.dirname(root), 'keys.csv'),
      }
      @options = [
        ['-h', '--help', 'Display this message', lambda { STDERR.puts o; exit(2) }],
        ['--keys FILE', String, 'Path to a csv file with the keymap', lambda {|a| @config[:keys] = a}],
      ]

      instance_eval(&block) if block_given?

      opt = OptionParser.new do |o|
        o.banner = @banner
        @options.each {|x| o.on(*x) }
      end
      opt.parse!

      unless ARGV.length == 1
        STDERR.puts opt
        exit(2)
      end

      target = ARGV.shift

      if File.exists? target
        begin
          socket = UNIXSocket.new(target)
        rescue SocketError => e
          STDERR.puts "Can't open UNIX socket: #{e}"
          exit(1)
        end
      else
        begin
          host, port = target.split(':')
          socket = TCPSocket.new(host, port.to_i)
        rescue => e
          STDERR.puts "Can't connect to #{host}:#{port}: #{e}"
          exit(1)
        end
      end

      app.new(socket, @config).run!
    end
  end
end

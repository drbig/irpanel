module IRPanel
  class Field
    attr_reader :x, :y, :data

    def self.str_diff(left, right)
      return [[0, right]] if left.length != right.length
      ret = Array.new
      current = false
      last = 0
      left = left.each_byte.to_a
      right.each_byte.with_index do |c,i|
        next if left[i] == c
        unless current
          current = [i, c.chr]
          last = i
        else
          if last == i-1
            current[1] += c.chr
            last = i
          else
            ret.push(current)
            current = [i, c.chr]
          end
        end
      end
      ret.push(current) if current
      ret.any? ? ret : nil
    end

    def initialize(name, x, y)
      @name = name
      @x = x
      @y = y
      @data = String.new
    end

    def set(data)
      if @data != data
        diff = Field.str_diff(@data, data)
        @data = data
        if diff.length == 1
          "g:#{@x+diff[0][0]}:#{@y}\np:#{diff[0][1]}\n"
        else
          "g:#{@x}:#{@y}\np:#{@data}\n"
        end
      else
        nil
      end
    end
  end
end

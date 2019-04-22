require 'erb'

class TestGen
  class Parser
    class BoundObject
      include ERB::Util
      attr_accessor :parent


      def initialize(parent, params: nil)
        self.parent = parent
      end

      def template
        self.parent.template
      end

      def options
        self.parent.options
      end

      def post
        # Called after a template is rendered
        #fail "@xyz not empty" unless @xyz.empty?
      end

      ######
      # Required for ERB to bind to this class successfuly
      def get_binding
        return binding()
      end

      #########
      # Test method
      def hello_world
        "Hello World!"
      end

      ######
      # Partials
      def partial(relpath)
        old_output = @output

        partial_path = File.join(File.dirname(template), relpath)
        partial_obj = Parser.new(self, partial_path, bound_obj: self)
        result = partial_obj.render

        @output = old_output
        return result
      end

      ######
      # Helpers
      CHAR_BIT   = 8
      SCHAR_MIN  = -127
      SCHAR_MAX  = 127
      UCHAR_MIN  = 0
      UCHAR_MAX  = 255
      CHAR_MIN   = UCHAR_MIN
      CHAR_MAX   = UCHAR_MAX
      SHRT_MIN   = -32767
      SHRT_MAX   = 32767
      USHRT_MIN  = 0
      USHRT_MAX  = 5535
      INT_MIN    = -32767
      INT_MAX    = 32767
      UINT_MIN   = 0
      UINT_MAX   = 65535
      LONG_MIN   = -2147483647
      LONG_MAX   = 2147483647
      ULONG_MIN  = 0
      ULONG_MAX  = 4294967295
      LLONG_MIN  = -9223372036854775807
      LLONG_MAX  = 9223372036854775807
      ULLONG_MIN = 0
      ULLONG_MAX = 18446744073709551615

      # Handle int/hex <-> float/double conversions
      def reinterpret_double(x)
        x = x.to_s(16).rjust(16, '0') unless x.is_a? String
        [x].pack('H*').unpack('G').first
      end

      def reinterpret_float(x)
        x = x.to_s(16).rjust(8, '0') unless x.is_a? String
        [x].pack('H*').unpack('g').first
      end

      def float_to_hex(x)
        [x].pack('g').unpack('H*').first
      end

      def double_to_hex(x)
        [x].pack('G').unpack('H*').first
      end
    end
  end
end

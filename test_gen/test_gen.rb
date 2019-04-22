require 'optparse'
require 'pathname'

# Require Lib folder
Dir.glob('lib/**/*.rb').each { |file| require_relative file }

# Main class
class TestGen
  #######
  # Options
  attr_accessor :options

  def parse_args()
    options = {
             in: 'templates',
            out: 'out',
           seed: 1,
      verbosity: 1
    }

    OptionParser.new do |opts|
      opts.banner = "Usage: test_gen.rb [options]"

      opts.on("-v", "--[no-]verbose", Integer, "Run verbosity") { |v| options[:verbosity] = v }
      opts.on("-s N", "--seed N", Integer, "Test seed") { |s| options[:seed] = s }
      opts.on("-i P", "--in P", String, "Input folder/file") { |i| options[:in] = i }
      opts.on("-o P", "--out P", String, "Output folder/file") { |o| options[:out] = o }
    end.parse!
  
    self.options = options
  end

  #######
  # Misc
  def parse_file(in_path, out_path)
    puts "Parsing '#{in_path}' to '#{out_path}'..."
    parser = Parser.new(self, in_path)
    parser.save(out_path)
  end

  def parse_files()
    if File.file?(options[:in])
      filepath = options[:in]
      filename = File.basename(filepath)

      out_path = options[:out]

      parse_file(filepath, out_path)
    elsif File.directory?(options[:in])
      Dir.glob("#{options[:in]}/**/*.erb").each do |filepath|
        next unless File.file?(filepath)

        filename = File.basename(filepath, '.erb')
        next if filename[0] == '_'

        file_pathname = Pathname.new(File.dirname(filepath))
        in_pathname = Pathname.new(options[:in])
        rel_pathname = file_pathname.relative_path_from(in_pathname)
        out_pathname = Pathname.new(options[:out])

        out_path = out_pathname.join(rel_pathname, filename)

        parse_file(filepath, out_path.to_s)
      end
    else
      Dir.glob(options[:in]).each do |filepath|
        next unless File.file?(filepath)

        filename = File.basename(filepath, '.erb')
        next if filename[0] == '_'

        out_path = File.join(options[:out], filename)

        parse_file(filepath, out_path)
      end
    end
  end

  def initialize()
    parse_args
  end
end

# Instantiate main object, call it
TestGen.new().parse_files

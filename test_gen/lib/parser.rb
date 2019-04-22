require 'erb'
require 'fileutils'

class TestGen
  class Parser
    attr_accessor :parent
    attr_accessor :bound_obj
    attr_accessor :template

    def options
      self.parent.options
    end

    def initialize(parent, filepath, bound_obj: nil)
      self.parent = parent
      self.template = File.read(filepath)
      self.bound_obj = bound_obj
    end

    def render
      srand options[:seed]
      self.bound_obj = BoundObject.new(self) if bound_obj.nil?
      ERB.new(self.template, 0, '%', "@output").result(self.bound_obj.get_binding)
    end

    def save(outpath)
      dirname = File.dirname(outpath)
      FileUtils.mkdir_p(dirname) unless File.directory?(dirname)

      File.open(outpath, 'w+') do |f|
        result = self.render
        self.bound_obj.post()
        f.write(result)
      end
    end
  end
end

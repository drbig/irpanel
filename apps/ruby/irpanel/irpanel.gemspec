# -*- encoding: utf-8 -*-

require File.expand_path('../lib/irpanel/version', __FILE__)

Gem::Specification.new do |gem|
  gem.name          = 'irpanel'
  gem.version       = IRPanel::VERSION
  gem.date          = Time.now
  gem.summary       = %q{Write IRPanel apps easily with Ruby.}
  gem.description   = %q{Simple app class for interfacing with the IRPanel daemon.}
  gem.license       = 'BSD'
  gem.authors       = ['Piotr S. Staszewski']
  gem.email         = 'p.staszewski@gmail.com'
  gem.homepage      = 'http://www.drbig.one.pl'

  gem.files         = `git ls-files`.split("\n")
  gem.executables   = gem.files.grep(%r{^bin/}).map{ |f| File.basename(f) }
  gem.test_files    = gem.files.grep(%r{^spec/})
  gem.require_paths = ['lib']

  gem.add_development_dependency 'rspec', '~> 0' 
  gem.add_development_dependency 'rubygems-tasks', '~> 0'
  gem.add_development_dependency 'yard', '~> 0'
end

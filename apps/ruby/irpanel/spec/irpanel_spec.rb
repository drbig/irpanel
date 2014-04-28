require 'rspec'
require 'irpanel'

describe IRPanel do
  it 'should have a VERSION constant' do
    subject.const_get('VERSION').should_not be_empty
  end
end

describe IRPanel::Field do
  field = nil

  it 'should initialize' do
    field = IRPanel::Field.new(:test, 1, 1)
    field.should_not be_nil
  end

  it 'should have accessible attributes' do
    field.x.should eq(1)
    field.y.should eq(1)
    field.data.should eq('')
  end

  it 'should calculate string differences' do
    IRPanel::Field.str_diff('foo', 'Foo').should eq([[0, 'F']])
    IRPanel::Field.str_diff('foo', 'fOO').should eq([[1, 'OO']])
    IRPanel::Field.str_diff('f',   'foo').should eq([[0, 'foo']])
    IRPanel::Field.str_diff('foo', 'FoO').should eq([[0, 'F'], [2, 'O']])
    IRPanel::Field.str_diff('foo', 'foo').should be_nil
  end

  it 'should generate proper commands' do
    field.set('one').should eq("g:1:1\np:one\n")
    field.set('oNe').should eq("g:2:1\np:N\n")
    field.set('ONE').should eq("g:1:1\np:ONE\n")
    field.set('ONE').should be_nil
    field.set('FREE').should eq("g:1:1\np:FREE\n")
  end
end

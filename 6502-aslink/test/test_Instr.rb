$:.unshift File.join(File.dirname(__FILE__), ".." )

require "test/unit"
require "6502-assembler.rb"



class TestInstr < Test::Unit::TestCase
    
  def setup
    # Nothing really
  end
 
  def teardown
    # Nothing really
  end
  
  def test_InstrBase
    i = InstrBase.new
    assert( i.instr_type == InstrBase:: UNINITIALIZED )
  end
  
  def test_Comment
    i = Comment.new
    assert( i.instr_type == InstrBase:: COMMENT )
    assert( i.comment == "")
    
    i = Comment.new(";This   is also a comment ")
    assert( i.instr_type == InstrBase:: COMMENT )
    assert( i.comment == ";This   is also a comment ")
  end
  
  def test_LabelHomeInstr
    assert_raise(ArgumentError ) do 
      i = LabelHomeInstr.new
    end
    
    i = LabelHomeInstr.new("break")
    assert( i.instr_type == InstrBase:: LABEL_HOME )
    assert( i.label_name == "break" )
    assert( i.label_addr == InstrBase:: UNINITIALIZED )
  end
  
  def test_LabelGotoInstr
    i = LabelGotoInstr.new
    assert( i.instr_type == InstrBase:: LABEL_GOTO )
    assert( i.label_addr == InstrBase:: UNINITIALIZED )
  end
  
  def test_Instr
    i = Instr.new
    assert( i.instr_type == InstrBase:: OPCODE )
    assert( i.sym_name == InstrBase:: UNINITIALIZED )
    assert( i.args == InstrBase:: UNINITIALIZED )
    assert( i.opcode == InstrBase:: UNINITIALIZED )
  end
  
  
end #class TestInstr < Test::Unit::TestCase
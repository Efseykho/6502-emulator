
if __FILE__ == $0
  print "6502-assembler called!"
end

require "tokenizer.rb"

class InstrBase
  attr_accessor :instr_type #type of instruction, ex: Comment, Instr, etc
  
  def initialize
    @instr_type = UNINITIALIZED
  end

  #define our symbology types
  UNINITIALIZED = -1 #generic unitialized value
  COMMENT = 1 #comment type
  OPCODE = 2 #opcode type
  LABEL_HOME = 3 #label home type
  LABEL_GOTO = 3 #label goto type
  
end #class InstrBase

class Comment < InstrBase
  attr_accessor :comment #the actual comment
  
  def initialize(comment = "")
    @instr_type = COMMENT
    
    @comment = comment
  end
  
  def to_s
    printf "Comment: #{@comment}\n"
  end
  def print
    to_s
  end
end #class Comment

#the label home  instr
class LabelHomeInstr < InstrBase
  attr_accessor :label_name #name of label
  attr_accessor :label_addr #where label points to once its been assembled
  
  def initialize(name)
    @instr_type = LABEL_HOME
    @label_name = name 
    @label_addr = UNINITIALIZED
  end
  
  def to_s
    printf "Label definition: #{@label_name}, to 0x#{@label_addr}\n"
  end
  def print
    to_s
  end
  
end #classLabelHomeInstr

#the label goto  instr
class LabelGotoInstr < InstrBase
    attr_accessor :label_addr #where label points to once its been assembled
    
  def initialize
    @instr_type = LABEL_GOTO
    @label_addr = UNINITIALIZED
  end
  
  def to_s
    printf "Label to 0x#{@label_addr}\n"
  end
  def print
    to_s
  end
  
end #class LabelGotoInstr

class Instr < InstrBase
  attr_accessor :sym_name #symbolic name, ex: LDA, STA, etc
  attr_accessor :args #arguments this opcode takes
  attr_accessor :opcode #hex-opcode of this instr

  #default initializer to invalid opcode
  def initialize
      @instr_type = OPCODE

      @sym_name = UNINITIALIZED
      @args = UNINITIALIZED
      @opcode = UNINITIALIZED
  end
    
  def to_s
    printf "Instruction #{@sym_name.to_s}"
    printf " opcode=0x#{@opcode}"
    if @args != UNINITIALIZED
      printf ", args="
      printf "[#{@args.join(", ")}]"
    end
    printf "\n"
  end
  def print
    to_s
  end
  
end #class Instr


class Assembler
  attr_accessor :instr #listing of all parsed insructions encountered
  attr_accessor :def_labels #listing of all defined labels in source
  
  def initialize
    @instr = nil #Array.new
    @def_labels =nil  # label_name => line number
    @tokenizer = nil
  end
  
  #input: token to consider
  #output: label, if it is a label, nil otherwise
  def is_label?(tok)
    #a label is defined as: 1st character = letter, followed by upto 5 chars/digits, followed by ":"
    return tok.chop if  ( (tok =~ /[A-Za-z]\w{0,5}:/) == 0) and tok[-1] == 58
    return nil
  end
  
  
  #TODO:
  #start parsing memory addressing modes
  #ex: preindexed indirect: ($20,X) or generally ([8bit number],{X,Y}), no w/s
  #def is pre_indexed_indirect
  #
  #end
  
  def is_comment?(tok)
    return tok if tok[0,1] == ";"
    return nil
  end
  
  def is_number?(tok)
    #check number format: correct types of digits
    if  tok[0] == 36 
      return nil if( (tok.sub("$","") =~ /[^A-Fa-f0-9]/) != nil)
    elsif tok[0] == 67 
      return nil if ( (tok.sub("C","") =~ /[^0-7]/) != nil)
    elsif tok[0] == 66 
      return nil if ( (tok.sub("B","") =~ /[^01]/) != nil)      
    elsif tok[0] >= 48 and tok[0] <= 57
      return nil if ( (tok =~ /[^0-9]/) != nil)   
    else
      #can raise exceptions here:
      return nil
    end
    
    return get_number_system(tok)
  end
  
  def is_8bit_number?(tok)
    ret = is_number?(tok)

    #check whether number fits in 8bits: max value of 255, unsigned
    return ret if ret != nil and ret <= 255 and ret >= 0
    return nil
  end

    def is_16bit_number?(tok)
    ret = is_number?(tok)

    #check whether number fits in 16bits: max value of 0xFFFF=65535b10, unsigned
    return ret if ret != nil and ret <= 65535 and ret >= 0
    return nil
  end
  
  #hex: $[0-9A-F]*
  #dec: [0-9]*
  #octal: C[0-7]*
  #binary: B[0-1]*
  #
  #output: number in proper decimal format
  def get_number_system(tok)
    ret = String.new(tok)
    if ret[0] == 36 # $
      ret[0] = ""
      return ret.to_i(16)
    elsif ret[0] == 67 # C
      ret[0] = ""
      return ret.to_i(8)
    elsif ret[0] == 66 # B
      ret[0] = ""
      return ret.to_i(2)
    else
      return ret.to_i(10)
    end
  end
  
  
  def assemble( filename = "sample.as" )
    @instr= Array.new
    @def_labels = Hash.new
    @tokenizer = Tokenizer.new(filename)
    
    while @tokenizer.has_more? do
      tok = @tokenizer.next
      printf "tok=#{tok} \n"
      
      #found comment
      if (str = is_comment?(tok)) != nil
        #@tokenizer.put_back(tok) : this is broken for now, maybe rewrite tokenizer to fix
        #tok = @tokenizer.get_line
        str = str + " " + @tokenizer.get_line
        print "found comment=#{str}\n"
        @instr.push( Comment.new(str) )
      elsif (str = is_label?(tok)) != nil
        #labels are defined by: char[char/digit]0-5':'
        printf "found label #{str}\n"
        label = LabelHomeInstr.new(str)
        @instr.push( label )
    
        #error check for label already existing
        raise "Label #{label.label_name} already defined at line #{@def_labels[label.label_name][0]}" if @def_labels.has_key?(label.label_name)
        
        #register label
        @def_labels[label.label_name] = [@tokenizer.curr_line, InstrBase::UNINITIALIZED]
      else
        raise "Invalid token \"#{tok}\" found at line=#{@tokenizer.curr_line}"
      end
    end #while @tokenizer.has_more? do
    
    
  end #def assemble( filename )
  
end #class Assembler


if __FILE__ == $0
  print "Assembler usage:\n"
  print "a = Assembler.new\n"
  print "a.assemble(\"sample.as\")\n"
  print "a.link_sym_labels\n"
  print "a.emit_instr(0x0600)\n"
  print "a.debug_emit(sample.asm)\n"
  print "a.emit_assembly(sample.asm)\n"
  print "\n\n"
  print "assemble() does the first pass-parsing\n"
  print "link_sym_labels() is a sanity check pass for symbols\n"
  print "emit_instr() is a 2-pass linking assuming all symbols are located\n"
  print "debug_emit() is optional debug step to output human-readable code\n"
  print "emit_assembly() outputs valid binary code + extra newline char\n"
end

require "tokenizer.rb"
require "gen_opcode.rb"


class InstrBase
  attr_accessor :instr_type #type of instruction, ex: Comment, Instr, etc
  attr_accessor :addr #memor addr of start of instr
  
  def initialize
    @instr_type = UNINITIALIZED
    @addr = UNINITIALIZED
  end

  #define our symbology types
  UNINITIALIZED = -1 #generic unitialized value
  COMMENT = 1 #comment type
  OPCODE = 2 #opcode type
  LABEL_HOME = 3 #label home type
  LABEL_GOTO = 4 #label goto type
  MACRO = 5 #such macros like dcb, etc
  
end #class InstrBase

class Comment < InstrBase
  attr_accessor :comment #the actual comment
  
  def initialize(comment = "")
    @instr_type = COMMENT
    
    @comment = comment
  end
  
  def to_s
    return "Comment: #{@comment}"
  end
  def print
    printf "#{to_s}\n"
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
    return "Label definition: #{@label_name}, to 0x#{@label_addr}"
  end
  def print
    printf "#{to_s}\n"
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
    return "Label to 0x#{@label_addr}"
  end
  def print
    printf "#{to_s}\n"
  end
  
end #class LabelGotoInstr

class Instr < InstrBase
  attr_accessor :sym_name #symbolic name, ex: LDA, STA, etc
  attr_accessor :args #arguments this opcode takes
  attr_accessor :opcode #hex-opcode of this instr
  attr_accessor :args_len #length of arguments to this instr, either {0,1,2}

  #default initializer to invalid opcode
  def initialize
      @instr_type = OPCODE

      @sym_name = UNINITIALIZED
      @args = UNINITIALIZED
      @opcode = UNINITIALIZED
      @args_len = 0
  end
    
  def to_s
    str = "Instruction #{@sym_name.to_s}"
    str += " opcode=0x#{@opcode}"
    if @args != UNINITIALIZED
      str += ", args="
      str +=  "[#{@args.to_s}]"
    end
    
    str += " addr=#{@addr}" if @addr != UNINITIALIZED
    str
  end
  def print
    printf "#{to_s}\n"
  end
  
end #class Instr

#implements any simple macros we need
class Macro < InstrBase
  attr_accessor :sym_name #symbolic name, ex: dcb, etc
  attr_accessor :args #arguments this macro takes
  
  #default initializer
  def initialize
      @instr_type = MACRO
      @sym_name = UNINITIALIZED
      @args = UNINITIALIZED
      
      @addr = UNINITIALIZED
  end
    
  def to_s
    str = "Macro #{@sym_name.to_s}"
    if @args != UNINITIALIZED
      str += ", args="
      str +=  "[#{@args.join(",")}]"
    end
    
    str += " addr=#{@addr}" if @addr != UNINITIALIZED
    str
  end
  def print
    printf "#{to_s}\n"
  end
end #class Macro


class Assembler
  attr_accessor :instr #listing of all parsed insructions encountered
  attr_accessor :def_labels #listing of all defined labels in source
  
  attr_accessor :instr_set #listing of entire instruction set of 6502 chip
  
  
  ######################
  #all different memory accessing modes
  IMPLIED = 0
  ACCUM = 1
  IMMEDIATE  = 2
  Z_PAGE = 3
  Z_PAGE_X = 4
  Z_PAGE_Y = 5
  IND_X = 6
  IND_Y = 7
  ABS_X =8
  ABS_Y = 9
  ABSOLUTE  = 10
  INDIRECT = 11
  RELATIVE = 12
  ######################
  
  def initialize
    @instr = nil #Array.new
    @def_labels =nil  # label_name => line number
    @tokenizer = nil
    
    #define instr set here:
    @instr_set = GenOpCode.new.emit_table
  end
  
  #input: token to consider
  #output: label, if it is a label, nil otherwise
  def is_label?(tok)
    #a label is defined as: 1st character = letter, followed by upto 10 chars/digits, followed by ":"
    return tok.chop if  ( (tok =~ /[A-Z_a-z]\w{0,10}:$/) == 0) and tok[-1] == 58
    return nil
  end
  
  def is_comment?(tok)
    return tok if tok[0,1] == ";"
    return nil
  end
  
  #input: token to test, can be in any memory format
  #output: nil, if not a number in any base, number base10, if a number
  def is_number?(tok)
    #check number format: correct types of digits
    if  tok[0] == 36 # $
      return nil if( (tok.sub("$","") =~ /[^A-Fa-f0-9]/) != nil)
    elsif tok[0] == 67 # C
      return nil if ( (tok.sub("C","") =~ /[^0-7]/) != nil)
    elsif tok[0] == 66 # B
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
  
  ######################
  #an aside on how we return from figuring out our instructions:
  #return is: [ A, B, C, D ]
  #defined as, A=type of addressing mode, IMMEDIATE -> INDIRECT_MEMORY, {0...10}
  #               B = {1,2}, size of instr in bits
  #               C = {X,Y}, register X or Y, if applicable, nil otherwise
  #               D = {Integer,String}, a number for the operand, a label string otherwise
  ######################
  def debug_addr_ret(ret)
    return nil if ret.size != 4 
    printf "Addr mode=#{ret[0]}, instr size=#{ret[1]}, index=#{ret[2]}, oper=#{ret[3]}\n"
  end
  
  #is token an imm. memory access
  #imm memory is only ever 1byte long
  #can't be a label either cause labels are 2bytes long
  def is_imm_mem?(tok)
    
    return nil if tok[0] != 35
    str = String.new(tok)
    str[0] = ""
    
    ret = is_8bit_number?(str)
    return nil if ret == nil
    
    #it is in fact an imm memory acces, fill out ret array
    return  [ IMMEDIATE, 1, nil, ret ]
  end
  

   #is token an direct. memory access
  #direct memory is either 1 byte(for zero page), 2bytes(absolute), label(will be sub'd for a 2byte addr)
  def is_direct_mem?(tok)
    str = String.new(tok)

    ret = is_8bit_number?(str)
    return [ Z_PAGE, 1, nil, ret ] if ret 
    
    ret = is_16bit_number?(str)
    return [ ABSOLUTE, 2, nil, ret ] if ret 
    
    #finally check labels
    ret = is_label?(str +":")
    return [ ABSOLUTE, 2, nil, ret ] if ret 
    
    return nil
  end
  
  #is token an pre-indexed indirect. memory addressing: (PP,X)
  #this memory accessing mode is only ever 1byte in length
  #in 6502 its only indexed by X-register
  #no labels are possible
  def is_pre_indexed_indirect_mem?(tok)
    str = String.new(tok)
    return nil if str[0] != 40 or str[-1] != 41
    
    str[0] = ""
    str[-1] = ""
    
    str = str.split(",")
    return nil if str.size != 2 or str[1].upcase != "X"
    
    ret = is_8bit_number?(str[0] )
    return [ IND_X, 1, :X, ret ] if ret
    
    return nil
  end
  
  #is token an post-indexed indirect. memory addressing: (PP),Y
  #this memory accessing mode is only ever 1byte in length
  #in 6502 its only indexed by Y-register
  #no labels are possible
  def is_post_indexed_indirect_mem?(tok)
    str = String.new(tok)
    str = str.split(",")
    return nil if str.size != 2 or str[1].upcase != "Y"
    
    str = str[0]
    return nil if str[0] != 40 or str[-1] != 41
    
    str[0] = ""
    str[-1] = ""
    
    ret = is_8bit_number?(str )
    return [ IND_Y, 1, :Y, ret ] if ret
    
    return nil
  end
  
  #is token an indexed indirect. memory addressing: PP{QQ},{X,Y}
  #this memory accessing mode can be 1 or 2 bytes in length
  #it can be indexed through either X or Y registers
  #labels are possible
  def is_indexed_indirect_mem?(tok)
    str = String.new(tok)
    str = str.split(",")
    
    return nil if str.size != 2 or (str[1].upcase != "Y" and str[1].upcase != "X")
    
    ret = is_8bit_number?(str[0] )
    if ret != nil
      return [ Z_PAGE_X, 1, str[1].to_sym, ret ] if str[1].upcase == "X"
      return [ Z_PAGE_Y, 1, str[1].to_sym, ret ]
    end
    
    ret = is_16bit_number?(str[0])
    if ret != nil
      return [ ABS_X, 2, str[1].to_sym, ret ] if str[1].upcase == "X"
      return [ ABS_Y, 2, str[1].to_sym, ret ]
    end
    
    #finally check labels
    ret = is_label?(str[0] +":")
    if ret != nil
       return [ ABS_X, 2, str[1].to_sym, ret ] if str[1].upcase == "X"
       return [ ABS_Y, 2, str[1].to_sym, ret ] 
    end
   
    return nil
  end
  
  #is token an indirect. memory addressing: PPQQ
  #this memory accessing mode is 2bytes in length
  #labels are possible
  #it is a special form of indexed addressing with index=0 and 2bytes in length
  def is_indirect_mem?(tok)
    str = String.new(tok)
    return nil if str[0] != 40 or str[-1] != 41
    
    str[0] = ""
    str[-1] = ""
    
    ret = is_16bit_number?(str)
    return [ INDIRECT, 2, nil, ret ] if ret
    
    #finally check labels
    ret = is_label?(str +":")
    return [ INDIRECT, 2, nil, ret ] if ret 
  end
  
  
  #figures out which memory access we've got
  #checks any and all labels to make sure they're registered before use
  def get_unified_mem_access(tok)
    arr_mem = [ "is_imm_mem?", 
                      "is_direct_mem?",
                      "is_indexed_indirect_mem?",
                      "is_pre_indexed_indirect_mem?",
                      "is_post_indexed_indirect_mem?",
                      "is_indexed_indirect_mem?",
                      "is_indirect_mem?",
                      ]

  arr_mem.each{ |i|
    #invoke each mem test method in turn, figure out if one of them passed
    ret = send(i.to_sym, String.new(tok))
    if ret != nil
      p "found valid instr-mode: #{ret}"
      return ret
    end
  }
  
  nil
  end

  def assemble( filename = "sample.as" )
    @instr= Array.new
    @def_labels = Hash.new
    #@undef_labels = Hash.new
    @tokenizer = Tokenizer.new(filename)
    
    while @tokenizer.has_more? do
      tok = @tokenizer.next
      printf "tok=#{tok} \n"
      
      #found comment
      if (str = is_comment?(tok)) != nil
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
      elsif (tok.downcase == "dcb") #found dcb macro
        str = @tokenizer.next
        raise "No input found for DCB macro" if str==nil
        str = str.split(",")
        
        m = Macro.new
        m.args = Array.new
        m.sym_name = "dcb"
        
        str.each{ |i|
          raise "Invalid input to dcb macro found: #{i}" if ! is_8bit_number?(i)
          m.args.push( is_8bit_number?(i) )
        }
        
        printf "created dcb macro #{m}"
        @instr.push(m)
      elsif ( @instr_set.has_key?(tok.upcase) )
        printf "found key #{tok} and val #{@instr_set[tok.upcase]}\n"
        
        #first, check if instr is implied
        #if instr uses implied addressing, no other addressing modes are avail
        if @instr_set[tok.upcase][IMPLIED] != nil
          instr = Instr.new
          instr.sym_name = tok.upcase
          instr.opcode = @instr_set[tok.upcase][IMPLIED]
          
          @instr.push( instr )
        else #in all other cases, we're gonna have to figure out which mem-acces mode we're in
          t = @tokenizer.peek
          p "peeked val=#{t}"
          
        
          #edge case: last instr in stream is accumulator addressing
          if t == nil and @instr_set[tok.upcase][ACCUM] != nil
            instr = Instr.new
            instr.sym_name = tok.upcase
            instr.opcode = @instr_set[tok.upcase][ACCUM]
            @instr.push( instr )
            
            @tokenizer.next
            next 
          end
          
          #deal with accumulator addressing here in general case
          #we ran up a case like:  asl\n tax
          #both are accumulator instr but they get interpreted as asl 'label16'
          #to fix, check that 't' is not a valid instr already
          if @instr_set[tok.upcase][ACCUM] != nil and @instr_set[t.upcase] != nil
            instr = Instr.new
            instr.sym_name = tok.upcase
            instr.opcode = @instr_set[tok.upcase][ACCUM]
            @instr.push( instr )
            p "found accumulator addressing problem!\n"
            
            next 
          end
          
          #figure out correct addressing mode
          ret = get_unified_mem_access(t)

          #no valid mem-acces mode found, is it accumulator addressing?
          if ret == nil and @instr_set[tok.upcase][ACCUM] != nil
            instr = Instr.new
            instr.sym_name = tok.upcase
            instr.opcode = @instr_set[tok.upcase][ACCUM]
            @instr.push( instr )
            
            next 
          end
          
          debug_addr_ret(ret)

          #if value is some kind of integer, life's easy!
          if ret[3].respond_to?(:div) 
            raise "Instr #{tok} does not have addressing mode=#{ret[0]} found at line=#{@tokenizer.curr_line}" if @instr_set[tok.upcase][ret[0]] == nil
            
            #p "**************:"
            
            instr = Instr.new
            instr.sym_name = tok.upcase
            instr.opcode = @instr_set[tok.upcase][ret[0]]
            instr.args = ret[3]
            instr.args_len = ret[1]
            @instr.push( instr )
            
            @tokenizer.next
            next
          end
          
          #otherwise, value is possibly a label and we must make a choice
          #do we allow unlinked labels for future linkings?
          #or is this a straight-up 1-pass assembler
          #for now, lets go with 1-pass, it should be simple to add later
          #
          #...dammit, looks like we have to get fancier and allow unlinked labels
          #
          ##error check for label already existing
          #raise "Undefined label #{ret[3]} found at line #{@tokenizer.curr_line}" if ! @def_labels.has_key?(ret[3])
          
          instr = Instr.new
          instr.sym_name = tok.upcase
          
          #yet another 'special case here'
          #ex: bne LABEL
          #bne uses relative addressing - 1byte addition ot PC but LABEL is 2bytes
          #solution: for now, store label at destination, when we figure out where LABEL is in mem,
          #subtract curr location from LABEL to get disp (must be positive, 1byte value)
          #
          #sidenote: if op has RELATIVE addressing, thats its only instr mode possible
          if ret[0] == ABSOLUTE and @instr_set[tok.upcase][RELATIVE] != nil
            instr.opcode = @instr_set[tok.upcase][RELATIVE] 
            instr.args_len = 1 #rellative addressing labels occupy 1 byte
            printf "found relative addressing #{@instr_set[tok.upcase][RELATIVE] }\n"
          else
            instr.opcode = @instr_set[tok.upcase][ret[0]] 
            instr.args_len = ret[1]
          end
          
          instr.args =LabelGotoInstr.new
          instr.args.label_addr = ret[3]
          
          @instr.push( instr )
            
          @tokenizer.next
          next
        end  #end-else #in all other cases, we're gonna have to figure out which mem-acces mode we're in
        

      else
        raise "Invalid token \"#{tok}\" found at line=#{@tokenizer.curr_line}"
      end
    end #while @tokenizer.has_more? do
  end #def assemble( filename )
  
  
  #emits a bytestream of 0x86 aligned bytes that should be able to be run
  #attempts to link 
  #currently we can compile code that has calls to undefined labels
  #we must catch it in the linking
  def emit_instr(base_addr = 0x0600)
    #by convention, stack is 0x0100-0x01FF
    #so we try tolay out our program starting from 0x0200
    #
    #errr.... turns out 6502asm.com starts userspace addrs at 0x0600
    
    #2-pass compilation:
    #first pass, we lay out all the instr except for goto-labels
    addr = base_addr
    
    @instr.each { |i|
      case i.instr_type
        when InstrBase::OPCODE
          #p "opcode #{i}"
          i.addr = addr
          
          #increment by number of instructions used: 1 for opcode + 1-2 for args
          addr = addr + 1 + i.args_len
        when InstrBase::LABEL_HOME
          #p "label #{i}"
          i.addr = addr
          i.label_addr = addr
          
          #also, need to update @def_labels structure for lookups later
          @def_labels[i.label_name][1] = addr   
        when InstrBase::MACRO
          #p "macro #{i.to_s}"       
          # set addr of macro to starting addr
          i.addr = addr
          
          # increment addr by length of macro's args
          addr = addr + i.args.length
          
          #actual opcode is captured in @args of macro, get it later
        else
          #p "else #{i.instr_type}"
      end
    }
    
    #second pass, we link in all the goto-labels
    @instr.each { |i|
      case i.instr_type
        when InstrBase::OPCODE
          if i.args.respond_to?(:instr_type)
            if i.args.instr_type == InstrBase::LABEL_GOTO
              
              #if we have a  relative addr, args_len = 1
              #add signed 1byte disp to instr. counter
              if i.args_len == 1
                disp =@def_labels[i.args.label_addr][1] - (i.addr + 2)
                i.args = disp
              else #replace label goto with mapped addr to it
                 i.args = @def_labels[i.args.label_addr][1]
              end
            
            else
              #raise exception here
              raise "Unknown opcode arg-type \"#{i.args}\" found"
            end
          end
        else
          #p "else #{i.instr_type}"          
      end
    }
        
        
    
  end
  
  #because we might have undefined labels, we're going to error check now
  #we still don't know physical addr yet so we'll just do a rough check
  #we want to call this AFTER assemble() but BEFORE emit_instr()
  def link_sym_labels
    @instr.each{ |i|
      if i.instr_type == InstrBase::OPCODE and i.args != nil and i.args.respond_to?(:label_addr) and  not @def_labels.has_key?(i.args.label_addr)
        raise "Reference to undefined label #{i.args.label_addr} found"
      end
    } 
    nil
  end

  
  #emits assembly
  #ASSUMPTION: all has been linked before this got called
  #this should be private
  def emit_assembly(fname = "output")
    out_arr = Array.new
    pack_str = String.new
    
    @instr.each{|i|
      if i.instr_type == InstrBase::OPCODE
        out_arr.push(i.opcode.to_i(16))
        pack_str = pack_str + "C"
        next if i.args_len == 0
        
        if i.args_len == 1
          out_arr.push(i.args) 
          pack_str = pack_str + "C"
        elsif i.args_len == 2
          #gotta remember to put it in correct order, i guess
          #ex: $31F6 -> F6,31, low-order byte is in lower memory, high-order byte in high memory
          out_arr.push(i.args) if 
          pack_str = pack_str +"S"
        else
          raise "Invalid opcode args found #{i}"
        end

      elsif i.instr_type == InstrBase::MACRO
        raise "Unknown macro found #{i.to_s}" if i.sym_name != "dcb" 
        #currently we only recognize dcb macro
        
        #simply emit a series of bytes as specified in @args of dcb macro
        i.args.each{ |i|
          out_arr.push(i)
          pack_str = pack_str + "C"
        }
      end # if i.instr_type == InstrBase::OPCODE
    }
    
    p "#{pack_str}"
    p "#{out_arr}"
  
    pack_str = out_arr.pack(pack_str)
    
    file = File.new(fname+".assl", "wb")
    file.puts(pack_str)
    file.close   
    
    out_arr
  end
  
  #emits user-readable output to check
  def debug_emit(fname = "output")
    file = File.new(fname+".debug", "w+")
    @instr.each{|i|
      file.puts "#{i.to_s}"
    }
    file.close    
  end
  
end #class Assembler

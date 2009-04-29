require 'hpricot'

#this reads opcode taken from here:
#http://www.xmission.com/~trevin/atari/6502_opcodes.html
#and creates an hash of all the opcodes available
class GenOpCode
  
  def initialize
    @GenOpCode_width = 40
    @GenOpCode_start = 54
  end
  
  def emit_table
    instr_set = Hash.new
    h = Hpricot(open("opcode_table"))
    arr = h.search("td")
    
    i = 0;
    j = (arr.size - @GenOpCode_start) / @GenOpCode_width
    
    while i < j do
      temp = arr[@GenOpCode_start +i * @GenOpCode_width,@GenOpCode_width]
      
      tarr = Array.new
      instr_name = temp.shift.inner_html

      temp.each_index{ |k|
        tarr[k.div(3)] = temp[k].inner_html if k%3 == 0 and temp[k].inner_html != ""
      }
      instr_set[ instr_name ] = tarr
    
      #other stuff happens
      i += 1
    end
    
    instr_set.delete_if{ |k,v| k.index("*")  != nil }
  end
  
  
end
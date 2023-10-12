#include "utils.h"

String rightJustify(int num, byte digits) {
    String numStr = String(num);
    int numDigits = numStr.length(); 
    
    // If the number of digits is less than the specified width, pad with spaces on the left side
    if (numDigits < digits) {
        int numSpaces = digits - numDigits;
        for (int i = 0; i < numSpaces; i++) {
            numStr = " " + numStr;
        }
    }
    
    return numStr;
}
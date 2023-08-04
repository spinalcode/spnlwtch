int stringLength(const char* text){
  int x=0;
  uint8_t numChars = strlen(text);
  for (uint8_t t = 0; t < numChars; t++) {
    uint8_t character = text[t] - 32;
    x+=(pgm_read_byte(&font01_widths[character])*gScale);
  }
  return x;
}

char* getSubstring(char* line, char* src) {
  // Find the word "src:" in the line
  char* wordStart = strstr(line, src);

  if (wordStart != NULL) {
    // Find the next occurrence of "
    char* startQuote = strchr(wordStart, '"');
  
    if (startQuote != NULL) {
      // Calculate the character number of the start quote
      int quoteStartIndex = startQuote - line;
  
      // Find the next occurrence of " after the first
      char* endQuote = strchr(startQuote + 1, '"');
  
      if (endQuote != NULL) {
        // Calculate the character number of the end quote
        int quoteEndIndex = endQuote - line;
  
        // Calculate the length of the substring
        int substringLength = quoteEndIndex - quoteStartIndex - 1; // Exclude " from the length
  
        // Create a new char array to store the substring
        char* substring = (char*) malloc(substringLength + 1); // +1 for the null terminator
  
        // Copy the substring into the new char array
        strncpy(substring, startQuote + 1, substringLength);
        substring[substringLength] = '\0'; // Add null terminator
  
        return substring;
      } else {
        Serial.println("No ending quote was found for 'src:'.");
      }
    } else {
      Serial.println("No starting quote was found for 'src:'.");
    }
  } else {
    Serial.println("The word 'src:' was not found in the line.");
  }

  return NULL;
}

void print(int x, int y, const char* text){
  int x1 = x;
  int fHeight = 0;
  int nextSpace = 0;
  uint8_t numChars = strlen(text);
  for (uint8_t t = 0; t < numChars; t++) {
    uint8_t character = text[t] - 32;
    switch(gFont){
      case 0:
        // there is no 0, that would be confusing, so continue to 1
      case 1:
        fHeight = font01[0][1];
        drawSprite(&font01[character][0], x1, y);
        nextSpace = (pgm_read_byte(&font01_widths[character])*gScale);
        if(x1 + nextSpace >= 144){
          x1=x;
          y+=(fHeight*gScale); // line height
        }else{
          x1+=nextSpace;
        }
        break;      
      case 2:
        fHeight = font02[0][1];
        drawSprite(&font02[character][0], x1, y);
        nextSpace = (pgm_read_byte(&font02_widths[character])*gScale);
        if(x1 + nextSpace >= 144){
          x1=x;
          y+=(fHeight*gScale); // line height
        }else{
          x1+=nextSpace;
        }
        break;      
      case 3:
        fHeight = font03[0][1];
        drawSprite(&font03[character][0], x1, y);
        nextSpace = (pgm_read_byte(&font03_widths[character])*gScale);
        if(x1 + nextSpace >= 144){
          x1=x;
          y+=(fHeight*gScale); // line height
        }else{
          x1+=nextSpace;
        }
        break;      
    }
  }
}




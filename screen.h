
#define WIDTH_8 18
#define WIDTH 144
#define HEIGHT 168

#define GREY 2
#define WHITE 1
#define BLACK 0

uint8_t gScale = 1;
uint8_t gCol = WHITE;
uint8_t GreyPattern[]={0,1,1,0};

int CS_PIN = 8;
int where = 0; // current offset of screen buffer
uint8_t sBuffer = 0; // 0 or 1 

char sBuff[2][WIDTH*HEIGHT/8]; // 1bpp buffer (8 pixels per byte)

#define _SHARPMEM_BIT_WRITECMD 0x80
#define _SHARPMEM_BIT_VCOM 0x40
#define _SHARPMEM_BIT_CLEAR 0x20

int vCom = 1;

inline unsigned char reverse_byte(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

void clearScreenBuffer(){
  // clear the gfx buffer.
  memset(sBuff[sBuffer], 0x00, 3024); // WIDTH*HEIGHT/8
}

void writeScreen(){
  digitalWrite(CS_PIN, HIGH);
  SPI.beginTransaction( SPISettings(8000000, LSBFIRST, SPI_MODE0) ); // 8mhz as fast as the nrf52840 will go, but that's 100fps, so that's OK.
  // add vCom to the 'send data' command
  SPI.transfer(_SHARPMEM_BIT_WRITECMD | (vCom ? _SHARPMEM_BIT_VCOM : 0));

  #define numLines 8
  uint8_t fullLine[numLines * 20]; // 8 lines of screen data
  uint16_t index = 0; // index of buffer to read from

  for(uint16_t line=1; line<=168; line+=8){
    
    for(uint16_t i=0; i<numLines; i++){
      index = (line + i - 1) * 18;
      fullLine[i * 20] = reverse_byte(line + i);
      for(int x=1; x<19; x++){
        fullLine[(i*20)+x] = sBuff[sBuffer][index + x - 1];
      }
      fullLine[i * 20 + 19] = 0x00;
    }
    SPI.transfer(&fullLine, numLines * 20);
  }

  SPI.transfer(0x00);
  digitalWrite(CS_PIN, LOW);
  vCom ^= 1;
//  gScale = 1;
}

inline void drawPixel(int x, int y, uint8_t color){
  if(x<0 || y<0 || x>=WIDTH || y>=HEIGHT) return;
  uint16_t x8 = x>>3;
  if(color > 1){
    color = GreyPattern[(x&1)+2*(y&1)];
  }
  if (color){
    sBuff[sBuffer][x8 + y * WIDTH_8] |= (128>>(x&7));
  }else{
    sBuff[sBuffer][x8 + y * WIDTH_8] &= ~(128>>(x&7));
  }
}

inline void fillRect(int x, int y, int width, int height, int colour){
  if(x<0 || y<0 || x>=WIDTH || y>=HEIGHT) return;
  for(int y1=y; y1<y+height; y1++){
    for(int x1=x; x1<x+width; x1++){
      drawPixel(x1, y1, colour);
    }
  }
}

void drawSprite(const uint8_t* sprite_data, int dx, int dy){
  int byteWidth = (pgm_read_byte(&sprite_data[0])+7)/8;
  for(int x=0; x<pgm_read_byte(&sprite_data[0]); x++){
    uint8_t x_d8 = x/8;
    int bitNum = 7-(x&7);
    for(int y=0; y<pgm_read_byte(&sprite_data[1]); y++){
      int byteNum = x_d8+(y*byteWidth);
      if((pgm_read_byte(&sprite_data[2+byteNum])>>bitNum)&1){
        if(gScale==1){
          drawPixel(dx + x, dy + y, gCol);
        }else{
          fillRect(dx + (x*gScale), dy + (y*gScale), gScale, gScale, gCol);
        }
      }
    }    
  }
}

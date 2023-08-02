// 
//  I like to use this button handling routine, it tells me if
//  a button had just been pressed, held or released
// 
//------------------------[ Button handling, very accurate ]------------------------
#define BTN_0 7
#define BTN_1 3
#define BTN_2 4
#define BTN_3 5

#define HELD 0
#define NEW 1
#define RELEASED 2
uint8_t CompletePad, ExPad, TempPad, myPad;
bool _BUp[3], _BDown[3], _BBack[3], _BSelect[3];

void UPDATEPAD(int pad, int var) {
      _BBack[pad] =       var &1;
        _BUp[pad] = (var >> 1)&1;
    _BSelect[pad] = (var >> 2)&1;
      _BDown[pad] = (var >> 3)&1;
}

void UpdatePad(int joy_code){
    ExPad = CompletePad;
    CompletePad = joy_code;
    UPDATEPAD(HELD, CompletePad); // held
    UPDATEPAD(RELEASED, (ExPad & (~CompletePad))); // released
    UPDATEPAD(NEW, (CompletePad & (~ExPad))); // newpress
}

uint8_t updateButtons(){
    uint8_t var = 0;
    if (    !digitalRead(BTN_0)) var |= 1;
    if (    !digitalRead(BTN_1)) var |= (1<<1);
    if (    !digitalRead(BTN_2)) var |= (1<<2);
    if (    !digitalRead(BTN_3)) var |= (1<<3);
    UpdatePad(var);
    return var;
}

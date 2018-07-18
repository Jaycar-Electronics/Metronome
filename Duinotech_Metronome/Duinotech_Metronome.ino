// Arduino based metronome using timer interrupts
// 20-240bpm = 0.3-4Hz => 250-3333ms cycle time using timer1 with 1024 prescale
// tone on timer2 using tone function
// change tempo/beats per accent with L/R / U/D
// save settings to eeprom with select (load on boot/reset)
// display tempo name from array

#include <EEPROM.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(8,9,4,5,6,7);
char temponames[][14]={
  "Larghissimo  ",
  "Grave        ",
  "Lento        ",
  "Adagio       ",
  "Andante      ",
  "Allegretto   ",
  "Allegro      ",
  "Vivace       ",
  "Presto       ",
  "Prestissimo  "
};

int tempos[]={25,46,64,76,113,121,169,177,201,241};

int tempo=60;     //60bpm
int acc=1;        //every beat accented
int lastkey=0;    //which key was pressed last cycle
unsigned long t0; //how long has key been held for
int accnote[]={523,262,262,262,262,262,262,262};  //C5,C4
int acccount=0; 

void setup() {
  loadfromeeprom();       //load settings
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("    Duinotech");   //splash screen
  lcd.setCursor(0, 1);
  lcd.print("    Metronome");
  delay(1000);
  lcd.clear();                  //normal display
  lcd.setCursor(0, 0);
  lcd.print("Tempo:    Acc:");
  dotemponame(tempo);
  pinMode(A5,OUTPUT);
  TMRsetup();
}

void loop() {
  int key=lcdkey();            //read keys
  unsigned int t;
  if((key>0)&&(lastkey==0)){   //if key pressed
    t0=millis();               //record when button pressed
    dokeys(key);               //act once straight away
    if(key==1){savetoeeprom();}//save values to EEPROM if select pressed
  }
  if((key>0)&&(lastkey==key)){  //if button is still held pressed
    if(millis()-t0>400){
      t0=t0+200;
      dokeys(key);              //repeat after delay if held down
    }
  }
  lastkey=key;            //save for next
  lcd.setCursor(6,0);     //update display
  if(tempo<100){lcd.write(' ');}else{lcd.write(((tempo/100)%10)+'0');}
  lcd.write(((tempo/10)%10)+'0');
  lcd.write(((tempo)%10)+'0');
  lcd.setCursor(14,0);
  lcd.write(((acc)%10)+'0');
  lcd.setCursor(15,1);
  t=TCNT1;                    //read timer1 counter
  if(t<2000){                 //visual display of time too (15625~1second)
    lcd.write('1'+acccount);
  }else{
    lcd.write(' ');   
  }
}

void dokeys(int key){         //act on key presses
  switch(key){
    case 2:tempo--;dotemponame(tempo);if(tempo<20){tempo=20;}break;   //left
    case 5:tempo++;dotemponame(tempo);if(tempo>240){tempo=240;}break; //right
    case 3:acc--;if(acc<1){acc=1;}break;                              //down
    case 4:acc++;if(acc>8){acc=8;}break;                              //up
  }
}

int keystate(){     //read key value
  int a=analogRead(0);
  if(a>831){return 0;}  //no key
  if(a>523){return 1;}  //select
  if(a>331){return 2;}  //left
  if(a>177){return 3;}  //down
  if(a>50){return 4;}   //up
  return 5;             //right
}

int lcdkey(){
  int a=keystate();               //check
  delay(5);                       //wait a bit
  if(a==keystate()){return a;}    //if there's no bounce
  return 0;                       //otherwise nothing
}

ISR(TIMER1_COMPA_vect) {    //gets triggered when timer1 overflows
  acccount++;                         //check accent
  if(acccount>=acc){acccount=0;}      
  OCR1A = ((F_CPU*60L)/1024) / tempo; //set top for next cycle
  tone(A3,accnote[acccount],20);      //start tone
}

void TMRsetup(){
  // Timer 1 set up as interrupt, only common thing this affects is servo library
  TCCR1A = 0;                       //CTC mode
  TCCR1B = _BV(WGM12) | _BV(CS10)|_BV(CS12);  //CTC mode, prescale=1024
  TCNT1 = 0;                        //reset counter
  OCR1A = ((F_CPU*60L)/1024) / tempo; //set top
  TIMSK1 = _BV(OCIE1A);             //set interrupt
}

void dotemponame(int bpm){
  lcd.setCursor(0,1);
  for(int i=0;i<10;i++){        //scan through list
    if(bpm<tempos[i]){lcd.print(temponames[i]);return;}  
  }  
}

void savetoeeprom(){
  EEPROM.write(0,tempo);            //tempo in eeprom#0
  EEPROM.write(1,acc);              //accent in eeprom#1
  lcd.setCursor(0,1);
  lcd.print("Setting saved");       //save message
  delay(1000);                      //wait a bit
  dotemponame(tempo);               //normal display
}

void loadfromeeprom(){
  tempo=EEPROM.read(0);                   //tempo in eeprom#0
  acc=EEPROM.read(1);                     //accent in eeprom#1
  if((tempo<20)||(tempo>240)){tempo=60;}  //load sensible defaults
  if((acc<1)||(acc>8)){acc=1;}            //load sensible defaults
}


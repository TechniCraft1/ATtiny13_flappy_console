//attiny 13 GAMING

#define BUTTON_PIN PB3

#define SCL PB2
#define SDA PB1

#define SSD1306_ADDR 0x3C


void i2c_start(void) {

    DDRB &= ~((1 << SDA) | (1 << SCL));
    PORTB |= (1 << SDA) | (1 << SCL);


    PORTB &= ~(1 << SDA);
    DDRB |= (1 << SDA);

    PORTB &= ~(1 << SCL);
    DDRB |= (1 << SCL); 
}

void i2c_stop(void) {

    PORTB &= ~(1 << SDA);
    DDRB |= (1 << SDA);


    DDRB &= ~(1 << SCL);
    PORTB |= (1 << SCL);

    DDRB &= ~(1 << SDA);
    PORTB |= (1 << SDA);
}


void i2c_write(uint8_t byte){
  for(uint8_t i = 0; i < 8; i++){
    if (byte & 0x80) {
      DDRB &= ~(1 << SDA);
      PORTB |= (1 << SDA);
    } else {
      PORTB &= ~(1 << SDA);
      DDRB |= (1 << SDA);
    }

    PORTB |= (1 << SCL);
    PORTB &= ~(1 << SCL);

    byte <<= 1;
  }

  DDRB &= ~(1 << SDA); 
  PORTB |= (1 << SDA);
  
  PORTB |= (1 << SCL);
  PORTB &= ~(1 << SCL);
}


static void ssd1306_cmd(uint8_t c) {
    i2c_start();
    i2c_write(SSD1306_ADDR << 1);
    i2c_write(0x00);
    i2c_write(c);
    i2c_stop();
}

static void ssd1306_set_pos(uint8_t x, uint8_t page) {
    i2c_start();
    i2c_write(SSD1306_ADDR << 1);
    i2c_write(0x00);
    i2c_write(0x21);
    i2c_write(x);
    i2c_write(127);
    i2c_write(0x22);
    i2c_write(page);
    i2c_write(7);    
} __attribute__((always_inline))

static void ssd1306_set_pos2(uint8_t x, uint8_t page) {
    //x += 32;
    ssd1306_cmd(0x21);
    ssd1306_cmd(x);
    ssd1306_cmd(127);
    ssd1306_cmd(0x22);
    ssd1306_cmd(page);
    ssd1306_cmd(7);
} __attribute__((always_inline))

static void ssd1306_init2(void) {
    ssd1306_cmd(0xAE); // display off
    ssd1306_cmd(0xD5); ssd1306_cmd(0x80); // clock
    ssd1306_cmd(0xA8); ssd1306_cmd(0x3F); // multiplex for 128x32 (0x1F), use 0x3F for 128x64
    ssd1306_cmd(0xD3); ssd1306_cmd(0x00); // offset
    ssd1306_cmd(0x40); // start line
    ssd1306_cmd(0x8D); ssd1306_cmd(0x14); // charge pump
    ssd1306_cmd(0x20); ssd1306_cmd(0x00); // horizontal addressing
    ssd1306_cmd(0xA1); // segment remap
    ssd1306_cmd(0xC0); // COM scan dec
    ssd1306_cmd(0xDA); ssd1306_cmd(0x12); // COM pins (for 128x64)
    ssd1306_cmd(0x81); ssd1306_cmd(0x7F); // contrast
    ssd1306_cmd(0xD9); ssd1306_cmd(0xF1); // precharge
    ssd1306_cmd(0xDB); ssd1306_cmd(0x40); // VCOM detect
    ssd1306_cmd(0xA4); // display follow RAM
    ssd1306_cmd(0xA6); // normal display


    for (uint8_t page = 0; page < 4; page++) {
        ssd1306_set_pos(0, page);
        i2c_start();
        i2c_write(SSD1306_ADDR << 1);
        i2c_write(0x40);
        for (uint8_t i = 0; i < 128; i++)
            i2c_write(0x00);
        i2c_stop();
    }
    ssd1306_cmd(0xAF); // display on
}

const uint8_t init_seq[] PROGMEM = {
  0xAE,
  0xD5, 0x80,
  0xA8, 0x3F,
  0xD3, 0x00,
  0x40,
  0x8D, 0x14,
  0x20, 0x00,
  0xA1,
  0xC0,
  0xDA, 0x12,
  0x81, 0x52,
  0xD9, 0xF1,
  0xDB, 0x40,
  0xA4,
  0xA6,
  //0xAF
};

static void ssd1306_init(void) {
  for (uint8_t i = 0; i < sizeof(init_seq); i++) {
    ssd1306_cmd(pgm_read_byte(&init_seq[i]));
  }
}

//state
uint8_t rng = 1; //seed
int8_t v = 0;
//int8_t y = 32;
uint8_t y = 32;
uint8_t pipe_x = 127;
uint8_t score = 0;

#define POS 42

static inline void pixel(uint8_t x, uint8_t y, uint8_t p) {
    uint8_t page = y >> 3;
    uint8_t bit  = p << (y & 7);

    ssd1306_set_pos(x, page);
    i2c_start();
    i2c_write(SSD1306_ADDR<<1);
    i2c_write(0x40);
    i2c_write(bit);
    i2c_stop();
} __attribute__((always_inline))
//more expensive than % for some reason
static inline uint16_t cheap_mod(uint16_t mod){
  while(mod > 9){
    mod - 10;
  }
  return mod;
}

#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

const uint8_t font[][5] PROGMEM = {
    {0b01111100, 0b10100010, 0b10010010, 0b10001010, 0b01111100}, // '0'
    {0x00,0x42,0xFE,0x02,0x00}, // '1'
    {0x42,0x86,0x8A,0x92,0x62}, // '2'
    {0x84,0x82,0xA2,0xD2,0x8C}, // '3'
    {0x18,0x28,0x48,0xFE,0x08}, // '4'
    {0xE4,0xA2,0xA2,0xA2,0x9C}, // '5'
    {0x3C,0x52,0x92,0x92,0x0C}, // '6'
    {0x80,0x8E,0x90,0xA0,0xC0}, // '7'
    {0x6C,0x92,0x92,0x92,0x6C}, // '8'
    {0b01100000, 0b10010010, 0b10010010, 0b10010010, 0b01111100} // 9'
};

const uint8_t graphic_score[31] PROGMEM = {
   0b0110001, 0b1001001, 0b1001001, 0b1001001, 0b1000110, 0b0000000, // S
   0b0111110, 0b1000001, 0b1000001, 0b1000001, 0b0100010, 0b0000000, // C
   0b0111110, 0b1000001, 0b1000001, 0b1000001, 0b0111110, 0b0000000, // O
   0b1111111, 0b1001000, 0b1001100, 0b1001010, 0b0110001, 0b0000000, // R
   0b1111111, 0b1001001, 0b1001001, 0b1001001, 0b1000001, 0b0000000, // E
   0b0000000
};

#define SSD1306_PWR_PIN PB0

ISR(PCINT0_vect) {
}

int main(void){
  //CLKPR = (1 << CLKPCE);
  //CLKPR = (1 << CLKPS1);

  ADCSRA &= ~(1 << ADEN);
  GIMSK |= (1 << PCIE);
  PCMSK |= (1 << PCINT3); //Button
 
  DDRB |= (1 << SSD1306_PWR_PIN);
  PORTB |= (1 << SSD1306_PWR_PIN);
  PORTB |= (1 << SDA);
  DDRB |= (1 << SCL);


  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sei();
  //digitalWrite(PB3, INPUT_PULLUP);
  //Serial.begin(9600);
  while(1){
  DDRB &= ~(1 << BUTTON_PIN);
  PORTB |= (1 << BUTTON_PIN);

  ssd1306_init();

  for (uint8_t page = 0; page < 8; page++) {
        ssd1306_set_pos(0, page);
        i2c_start();
        i2c_write(SSD1306_ADDR << 1);
        i2c_write(0x40);
        for (uint8_t i = 0; i < 128; i++) i2c_write((page == 7) ? 0x03 : 0x00);
        i2c_stop();
  }
  ssd1306_cmd(0xAF); // display on
  
  // load score graphic
  ssd1306_set_pos(8, 7);
  i2c_start();
  i2c_write(SSD1306_ADDR << 1);
  i2c_write(0x40);
  for(uint8_t i = 0; i < 31; i++){
    i2c_write(pgm_read_byte(&graphic_score[i]));
  }
  i2c_stop();
  while(1){
    //v = -1;
    //if(y == 0) v++;
    //if(!(PINB & 0b00001000)) v = 1;

    //v -= 1;

    //for(uint8_t b = 0; b < 7; b++){
    //  ssd1306_set_pos(pipe_x, b);
    //  i2c_start();
    //  i2c_write(SSD1306_ADDR<<1);
    //  i2c_write(0x40);
    //  i2c_write(((b >> 1) == (rng & 0x3)) ? 0x00 : 0xFF);
    //  i2c_stop();
    //}
    static uint8_t frame = 0;
    frame = !frame;
    if(frame == 0){
    frame = 0;

    for(uint8_t b = 0; b < 7; b++){
      if((b >> 1) != (rng & 0x3)){
        ssd1306_set_pos(pipe_x, b);
        i2c_start();
        i2c_write(SSD1306_ADDR<<1);
        i2c_write(0x40);
        i2c_write(0xFF);
        i2c_stop();
      }
    }
    uint8_t calc = pipe_x;
    calc += 2;
    if(calc == 128) calc = 1;
    for(uint8_t b = 0; b < 7; b++){
      ssd1306_set_pos(calc, b);
      i2c_start();
      i2c_write(SSD1306_ADDR<<1);
      i2c_write(0x40);
      i2c_write(0x00);
      i2c_stop();
    }
    }
    //if (v > 1) v = 1;
    //if (v < -1) v = -1;

    pixel(POS, y, 0x00);
    uint8_t d = 0;
    //y += v;
    if(y != 1) y -= 1;
    if(!(PINB & 0b00001000)) y += 2;
    

    //if (y < 0) y = 0;
    if (y > 55) y = 55;

    pixel(POS, y, 0x01);
    
    
    pipe_x -= 1;
    if(pipe_x == 0) {
      pipe_x = 127;
      //rng ^= rng<<3; 
      //rng ^= rng>>5; 
      //rng ^= rng<<1;
      //rng &= 0x07; 
      rng += 13;
      rng ^= (rng >> 3);
      
    }
    //Serial.print(y >> 3);
    //Serial.print(rng);
    //Serial.println(pipe_x);
    if((pipe_x == POS) && ((y >> 4) != (rng & 0x3))){
      score = 0;
      for(uint32_t i = 0; i < 80000; i++){ asm("nop\nnop\n\n");}
      //uint32_t i;
      //while(i < 580000 && (PINB & 0b00001000)) { asm("nop\n\n"); i++; }
      //while(PINB & 0b00001000);

      //uint32_t i = 0;
      //while(i < 500000 && (PINB & 0b00001000)){
      //  i++;
      //}
      //if(i >= 499999){
      PORTB = 0b00001000;
      sleep_enable();
      sleep_cpu();

      sleep_disable();
      PORTB |= (1 << SSD1306_PWR_PIN); // turn on display power
      for(uint32_t i = 0; i < 22000; i++) asm("nop\n\n");
      break; // redo init and clear after wakeup
      //}
      
      #define OFFSET 39

      ssd1306_set_pos(OFFSET, 7);
      i2c_start();
      i2c_write(SSD1306_ADDR<<1);
      i2c_write(0x40);
      for (uint8_t i = 0; i < (128 - OFFSET); i++) {
        i2c_write(0x03);
      }
      i2c_stop();
    } else if((pipe_x == POS)){
      score++;
      
      uint8_t number = score;
      uint8_t x = 53;
      while(number != 0){
        ssd1306_set_pos(x, 7);
        i2c_start();
        i2c_write(SSD1306_ADDR<<1);
        i2c_write(0x40);
        for (uint8_t i = 0; i < 5; i++) {
          i2c_write(pgm_read_byte(&font[number % 10][i]));
        }
        number = number / 10;
        x -= 5;
      }
      i2c_stop();
    }
    //for(uint16_t i = 0; i < 18000; i++)asm("nop\n\n");
  }
  }
}
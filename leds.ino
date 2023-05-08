void setup() {
  // put your setup code here, to run once:
  *port_l &= 0b11110001; // set leds to be off by default
  *port_l |= 0b00000001; // set except yellow :)
}

//put these in each specific case:
// turn on green
*port_l &= 0b11110010; //off
*port_l |= 0b00000010; //on

// turn on blue  
*port_l &= 0b11110100; 
*port_l |= 0b00000100;

// turn on red
*port_l &= 0b11111000; 
*port_l |= 0b00001000;


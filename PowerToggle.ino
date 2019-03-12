 void setup() {
  int powerToggle = 0;
}

void loop() {
  if (irbutton == 12){
    if (powerToggle == 0){
      serial.print ("<1>");
      powerToggle = 1;
    }
    else {
      serial.print ("<0>");
      powerToggle = 0;
      }
  }
}



//   if (powerToggle == 1){
//     serial.print ("<0>");
//   }
//   else {
//     serial.print ("<1>");
//     powerToggle = 0
//     return
//   }

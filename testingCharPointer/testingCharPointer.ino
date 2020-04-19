void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}


char * foo (){
  char * buf = (char *) malloc (666);
  strcpy (buf, "But does it get goat's blood out?");
  return buf;
} 

void loop (){
  char * p = foo ();
  Serial.println (p);
  free (p);
  delay(5000);
} 

#include <16F886.h>

#device ADC=10 *=16  

#FUSES NOWDT                    //No Watch Dog Timer
#FUSES PUT                      //Power Up Timer
#FUSES NOMCLR                   //Master Clear pin not enabled
#FUSES NOPROTECT                //Code not protected from reading
#FUSES NOCPD                    //No EE protection
#FUSES BROWNOUT                 //Brownout reset
#FUSES IESO                     //Internal External Switch Over mode enabled
#FUSES FCMEN                    //Fail-safe clock monitor enabled
#FUSES NOLVP                    //No low voltage prgming, B3(PIC16) or B5(PIC18) used for I/O
#FUSES NODEBUG                  //No Debug mode for ICD
#FUSES NOWRT                    //Program memory not write protected
#FUSES BORV40                   //Brownout reset at 4.0V
#FUSES RESERVED                 //Used to set the reserved FUSE bits
#FUSES INTRC_IO 

#use delay(clock=8M)

#use rs232(baud=9600,parity=N,xmit=PIN_C6,rcv=PIN_C7,bits=8)

volatile unsigned int rate[10]; // array to hold last ten IBI values
volatile unsigned long sampleCounter = 0; // used to determine pulse timing
volatile unsigned long lastBeatTime = 0; // used to find IBI
volatile long int P = 512; // used to find peak in pulse wave, seeded
volatile long int T = 512; // used to find trough in pulse wave, seeded
volatile long int thresh = 512; // used to find instant moment of heart beat, seeded
volatile unsigned int amp = 100; // used to hold amplitude of pulse waveform, seeded
volatile boolean firstBeat; // used to seed rate array so we startup with reasonable BPM
volatile boolean secondBeat; // used to seed rate array so we startup with reasonable BPM

volatile unsigned int BPM; // int that holds raw Analog in 0. updated every 2mS
volatile unsigned int16 Signal; // holds the incoming raw data
volatile long int IBI = 600; // int that holds the time interval between beats! Must be seeded!
volatile boolean Pulse; // "True" when User's live heartbeat is detected. "False" when not a "live beat".
volatile boolean QS; // becomes true when finds a beat.
long int N;
unsigned int runningTotal = 0;
int i = 0;

unsigned int16 adc_value = 0;

void calculate_heart_beat(unsigned int16 adc_value){

   Signal = adc_value;
   sampleCounter += 2;
   N = sampleCounter - lastBeatTime;
   
   //find trough of the pulse wave
   if (Signal < thresh && N > (IBI / 5)*3) { // avoid dichrotic noise by waiting 3/5 of last IBI
        if (Signal < T) { // T is the trough
            T = Signal; // keep track of lowest point in pulse wave
        }
    }
    
    //find peak of the pulse wave
    if (Signal > thresh && Signal > P) { // thresh condition helps avoid noise
        P = Signal; // P is the peak
    } // keep track of highest point in pulse wave
    
    if(N > 250){
      if ((Signal > thresh) && (Pulse == false) && (N > (IBI / 5)*3)){
         Pulse = true; // set the Pulse flag when we think there is a pulse
         IBI = sampleCounter - lastBeatTime; // measure time between beats in mS
         lastBeatTime = sampleCounter; // keep track of time for next pulse
         
         if (secondBeat) { // if this is the second beat, if secondBeat == TRUE
              secondBeat = false; // clear secondBeat flag
              
              for (i = 0; i <= 9; i++) { // seed the running total to get a realisitic BPM at startup
                 rate[i] = IBI;
              }
         }

         if (firstBeat) { // if it's the first time we found a beat, if firstBeat == TRUE
              firstBeat = false; // clear firstBeat flag
              secondBeat = true; // set the second beat flag
              return; // IBI value is unreliable so discard it
         }
         
         runningTotal = 0; // clear the runningTotal variable
         i = 0;
         for (i = 0; i <= 8; i++) { // shift data in the rate array
             rate[i] = rate[i + 1]; // and drop the oldest IBI value
             runningTotal += rate[i]; // add up the 9 oldest IBI values
         }

         rate[9] = IBI; // add the latest IBI to the rate array
         runningTotal += rate[9]; // add the latest IBI to runningTotal
         runningTotal /= 10; // average the last 10 IBI values
         BPM = 60000 / runningTotal; // how many beats can fit into a minute? that's BPM!
         QS = true; // set Quantified Self flag
      }
    }
    
    
    if (Signal < thresh && Pulse == true) { // when the values are going down, the beat is over
        Pulse = false; // reset the Pulse flag so we can do it again
        amp = P - T; // get amplitude of the pulse wave
        thresh = amp / 2 + T; // set thresh at 50% of the amplitude
        P = thresh; // reset these for next time
        T = thresh;
    }

    if (N > 2500) { // if 2.5 seconds go by without a beat
        thresh = 512; // set thresh default
        P = 512; // set P default
        T = 512; // set T default
        lastBeatTime = sampleCounter; // bring the lastBeatTime up to date
        firstBeat = true; // set these to avoid noise
        secondBeat = false; // when we get the heartbeat back
    }
    
}

void main()
{
   Pulse = FALSE;
   QS = FALSE;
   firstBeat = TRUE;
   secondBeat = FALSE;
   setup_adc_ports(sAN0); // setup PIN A0 as analog input 
   setup_adc( ADC_CLOCK_INTERNAL ); 
   set_adc_channel(0); // set the ADC chaneel to read 
   delay_us(100);

   while(1) {
      adc_value = read_adc();
      calculate_heart_beat(adc_value);
      if(QS == true){
         QS == false;
         printf("BPM - %u\r\n", BPM); 
      }
      delay_ms(20);   
   }
}
// Wait for more debug

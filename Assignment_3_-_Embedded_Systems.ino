// define pins 
#define sigB 13       // pin 13 to output watchdog
#define LED 14        // pin 14 to indicate error
#define button1 33    // pin 33 to check button press
#define aIN 12        // pin 12 to read analogue signal
#define pIN 25        // pin 25 to read digital signal
#define task2demo 26  // pin 26 to test execution time of task 2

// define rates of task in ms
// calculated period from frequency
#define t1 4      // SignalB repeats every 1300+2500+8 = 3808 microseconds ~4 ms
#define t2 200    // 5Hzb 
#define t3 1000   // 1Hz
#define t4 42     // 24Hz
#define t5 42     // 24Hz
#define t6 100    // 10Hz
#define t7 333    // 3Hz
#define t8 333    // 3Hz
#define t9 5000   // 0.3Hz

// define variables
bool b1 = false;
float freqIN = 0;
float analogueIN = 0;
float analogues[4];
float average_analogueIN;
float average_analogues[4];
int error_code;
static TaskHandle_t task_1 = NULL;
static TaskHandle_t task_2 = NULL;
static TaskHandle_t task_3 = NULL;
static TaskHandle_t task_4 = NULL;
static TaskHandle_t task_5 = NULL;
static TaskHandle_t task_6 = NULL;
static TaskHandle_t task_7 = NULL;
static TaskHandle_t task_8 = NULL;
static TaskHandle_t task_9 = NULL;

// Settings Globals
static const uint8_t msg_queue_len = 5;
static QueueHandle_t msg_queue;
static SemaphoreHandle_t mutex;


// define pin functionalities
// arrays
// baud rate
// create FreeRTOS tasks
void setup() {
  Serial.begin(38400);
  analogues[0] = 0;
  analogues[1] = 0;
  analogues[2] = 0;
  analogues[3] = 0;
  pinMode(sigB,OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(button1, INPUT);
  pinMode(pIN, INPUT);
  pinMode(aIN, INPUT);

  // Create Mutex
  mutex = xSemaphoreCreateMutex();

  // Create queue
  msg_queue = xQueueCreate(msg_queue_len, sizeof(int));
  
  xTaskCreate(
      task1,    // Function to be called
      "task1",  // Name of task
      1024,     // Stack size (bytes)
      NULL,     // Parameter to pass to function
      1,        // Task priority (0-25)
      &task_1); // Task Handle

  xTaskCreate(task2, "task2", 1024, NULL, 1, &task_2);
  xTaskCreate(task3, "task3", 1024, NULL, 1, &task_3);
  xTaskCreate(task4, "task4", 1024, NULL, 1, &task_4);
  xTaskCreate(task5, "task5", 1024, NULL, 1, &task_5);
  xTaskCreate(task6, "task6", 1024, NULL, 1, &task_6);
  xTaskCreate(task7, "task7", 1024, NULL, 1, &task_7);
  xTaskCreate(task8, "task8", 1024, NULL, 1, &task_8);
  xTaskCreate(task9, "task9", 1024, NULL, 1, &task_9);
}

// 50us pulse
void task1(void *parameter)  // SignalB repeats every 1300+2500+8 = 3808 microseconds ~4 ms
{
  for(;;)
  {
    digitalWrite(sigB, HIGH);
    vTaskDelay(0.05 / portTICK_PERIOD_MS); // expects the no. of ticks not the no. of ms
    digitalWrite(sigB, LOW);
    vTaskDelay(t1/portTICK_PERIOD_MS);
  }
}

// Read digital value from PIN button1
void task2(void *parameter)
{
  for(;;)
  {
    //digitalWrite(task2demo, HIGH);
    b1 = digitalRead(button1);
    vTaskDelay(t2/portTICK_PERIOD_MS);
    //digitalWrite(task2demo, LOW);
  }
}

// measure square wave input
// calculate frequency from pulse length (period)
void task3(void *parameter)
{
  for(;;)
  {
    float peak;
    peak = pulseIn(pIN, LOW);
    freqIN = 100000*2.6 / peak*2;
    vTaskDelay(t3/portTICK_PERIOD_MS);
  }
}

// store analogue values into arrays
// use for loop to cycle through
void task4(void *parameter)
{
  for(;;)
  {
    for(int i=1; i<4; i++)
    {
      analogues[i-1] = analogues[i];
    }
    analogues[3] = analogRead(aIN);
    vTaskDelay(t4/portTICK_PERIOD_MS);
  }
}

// calculates average analogue readings from previous tasks
void task5(void *parameter)
{
  for(;;)
  {
    average_analogueIN = 0;
    // loop to find some of values
    for(int i=0; i<4; i++)
    {
      average_analogueIN += analogues[i];
    }
    average_analogueIN = average_analogueIN /4; // divide by total of readings to obtain average reading
    if(xQueueSend(msg_queue, (void *) &average_analogueIN, 10) != pdPASS)
    {
      //Serial.print("Queue is full!\n");
    }
    vTaskDelay(t5/portTICK_PERIOD_MS);
  }
}

// this line of code waits for one clock cycle, 1000 times
// ESP32 has an internal 8MHz oscillator, thus, 1000 clock cycles = 1000/8000000 = 0.000125s
void task6(void *parameter)
{
  for(;;)
  {
    for(int i=0; i<1000; i++)
    {
      __asm__ __volatile__ ("nop");
    }
    vTaskDelay(t6/portTICK_PERIOD_MS);
  }
}

// define a value that when crossed enables error value
void task7(void *parameter)
{
  for(;;)
  {
    if(xQueueReceive(msg_queue, (void *) &average_analogueIN, 0) == pdTRUE)
    {
      if((average_analogueIN * 3.3 / 4095) > (3.3/2))
      {
        error_code = 1;
      } else {
        error_code = 0;
      }
    }
    vTaskDelay(t7/portTICK_PERIOD_MS);
  }
}

// visualize error code through an LED
void task8(void *parameter)
{
  for(;;)
  {
    digitalWrite(LED, error_code);
    vTaskDelay(t8/portTICK_PERIOD_MS);
  }
}

// print values to serial monitor
void task9(void *parameter)
{
  for(;;)
  {
    if(xSemaphoreTake(mutex, 0 ) == pdTRUE)
    {
      if(b1 == HIGH)
      {
        Serial.print("\nButton \tInput \tFrequency \n");
        Serial.print(b1);
        Serial.print(", \t");
        Serial.print(freqIN);
        Serial.print(", \t");
        Serial.print((average_analogueIN * 3.3 / 4095));
        Serial.print("\n");
      }
      xSemaphoreGive(mutex);
    }
    vTaskDelay(t9/portTICK_PERIOD_MS);
  }
}

void loop() {
  }
/*

       ___------__
 |\__-- /\       _-
 |/    __      -
 //\  /  \    /__
 |  o|  0|__     --_
 \\____-- __ \   ___-
 (@@    __/  / /_
    -_____---   --_
     //  \ \\   ___-
   //|\__/  \\  \
   \_-\_____/  \-\
        // \\--\|   "gotta go fast"
   ____//  ||_
  /_____\ /___\
______________________ */

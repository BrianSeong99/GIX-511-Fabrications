#include <Kalman.h>
#include <Wire.h>
#include <Math.h>
#include <FastLED.h>

#define LED_PIN     5
#define LED_POWER   13  
#define NUM_LEDS    72
#define BRIGHTNESS  255
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define SWITCH_HIGH 8
#define LEFT_SWITCH 6
#define RIGHT_SWITCH 7
#define DELAY 100 // frequency = 1000/DELAY
#define QUEUE_LEN 5

#define DEFAULT_LED 0
#define RAIN_DROP 1
#define ARROW 2
#define HEART 3
#define GIXSOLID 4
#define GIXCOLOR 5
#define WHITESCREEN 100
#define BLACKSCREEN 101
#define TOTAL_MODE_SIZE 6

#define WAVE_DETECTION_MODE   leftswitch && !rightswitch
#define DISPLAY_MODE          !leftswitch && rightswitch
#define BLACKSCREEN_MODE      leftswitch && rightswitch

int mode = 1;

CRGB leds[NUM_LEDS];

int pitch_queue[QUEUE_LEN] = {0,0,0,0,0};
int row_queue[QUEUE_LEN] = {0,0,0,0,0};
int cur_ptr = 0;
int LEFT_PITCH_BAR = 120;
int RIGHT_PITCH_BAR = -120;

float fRad2Deg = 57.295779513f; //将弧度转为角度的乘数
const int MPU = 0x68; //MPU-6050的I2C地址
const int nValCnt = 7; //一次读取寄存器的数量

const int nCalibTimes = 200; //校准时读数的次数
int calibData[nValCnt]; //校准数据

unsigned long nLastTime = 0; //上一次读数的时间
float fLastRoll = 0.0f; //上一次滤波得到的Roll角
float fLastPitch = 0.0f; //上一次滤波得到的Pitch角
Kalman kalmanRoll; //Roll角滤波器
Kalman kalmanPitch; //Pitch角滤波器

int next_gesture = 0; // 0 for left, 1 for right
int leftswitch = 0;
int rightswitch = 0;

uint8_t updates_per_second[6] = {
  100, 20, 10, 1, 1, 100,
}; // access via `mode`

uint8_t RainPattern[6*12] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

bool NextRainRow[6] = {false,false,false,false,false,false};

const uint8_t RainPaletteSize = 4;

extern const CRGB RainPalette[RainPaletteSize] = {
  CRGB::Black, CRGB::MidnightBlue, CRGB::Blue, CRGB::Teal,
};

extern const uint8_t ArrowPattern[6*12] = {
  0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
};

extern const CRGB ArrowPalette[2] = {
  CRGB::Black, CRGB::Green, 
};

extern const uint8_t HorizontalHeartPattern[2][6*12] = {
  {
    0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
    0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
  },
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  }
};

extern const CRGB HeartPalette[2] = {
  CRGB::Black, CRGB::Red, 
};

extern const uint8_t GIXPattern[6*12] = {
  1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1,
  1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 
  1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0,
  1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0,
  1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1,
  1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1,
};

extern const CRGB GIXPalette[2] = {
  CRGB::Black, CRGB::Yellow, 
};

void setup() {
  Serial.begin(9600); //初始化串口，指定波特率
  Wire.begin(); //初始化Wire库
  WriteMPUReg(0x6B, 0); //启动MPU6050设备

  Calibration(); //执行校准
  nLastTime = micros(); //记录当前时间
  
  pinMode(SWITCH_HIGH,OUTPUT); 
  digitalWrite(SWITCH_HIGH,HIGH);

  pinMode(LEFT_SWITCH,INPUT); 
  pinMode(RIGHT_SWITCH,INPUT); 
  
  pinMode(LED_POWER,OUTPUT); 
  digitalWrite(LED_POWER,HIGH);

  delay( 3000 ); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );

}

void loop() {
  leftswitch = digitalRead(LEFT_SWITCH);
  rightswitch = digitalRead(RIGHT_SWITCH);

  static uint8_t motionIndex = 0;
  motionIndex = motionIndex + 1; /* motion speed */
  static int pitch_gesture = 0;
  static bool change_pattern = false;

  if(WAVE_DETECTION_MODE){
    if (mode != HEART) mode = HEART;
    // Serial.print("WAVE_DETECTION_MODE\n");
    // delay(DELAY); 
    int readouts[nValCnt];
    ReadAccGyr(readouts); //读出测量值
    float realVals[7];
    Rectify(readouts, realVals); //根据校准的偏移量进行纠正
    //计算加速度向量的模长，均以g为单位
    float fNorm = sqrt(realVals[0] * realVals[0] + realVals[1] * realVals[1] + realVals[2] * realVals[2]);
    float fRoll = GetRoll(realVals, fNorm); //计算Roll角
    if (realVals[1] > 0) {
      fRoll = -fRoll;
    }
    float fPitch = GetPitch(realVals, fNorm); //计算Pitch角
    if (realVals[0] < 0) {
      fPitch = -fPitch;
    }

    //计算两次测量的时间间隔dt，以秒为单位
    unsigned long nCurTime = micros();
    float dt = (double)(nCurTime - nLastTime) / 1000000.0;
    //对Roll角和Pitch角进行卡尔曼滤波
    float fNewRoll = kalmanRoll.getAngle(fRoll, realVals[4], dt);
    float fNewPitch = kalmanPitch.getAngle(fPitch, realVals[5], dt);
    //跟据滤波值计算角度速
    float fRollRate = (fNewRoll - fLastRoll) / dt;
    float fPitchRate = (fNewPitch - fLastPitch) / dt;
    
    //更新Roll角和Pitch角
    fLastRoll = fNewRoll;
    fLastPitch = fNewPitch;
    //更新本次测的时间
    nLastTime = nCurTime;

    // detect gesture
    pitch_gesture = CheckPitch();
    if(pitch_gesture==1){
      Serial.print("PITCH GESTURE: 1");
      PosPrinter(1);
      mode = HEART;
    }
    else if(pitch_gesture==2){
      Serial.print("PITCH GESTURE: 2");
      PosPrinter(2);
      mode = HEART;
    }
    Serial.print("Roll:");
    Serial.print(fNewRoll); Serial.print('(');
    Serial.print(fRollRate); Serial.print("),\tPitch:");
    Serial.print(fNewPitch); Serial.print('(');
    Serial.print(fPitchRate); Serial.print(")\n");
    delay(DELAY); 

    pitch_queue[cur_ptr] = fPitchRate;
    row_queue[cur_ptr] = fRollRate;
    cur_ptr = (cur_ptr+1)%QUEUE_LEN;
  } else if (DISPLAY_MODE){
    // Serial.print("DISPLAY_MODE\n");
    // delay(DELAY); 
    if (change_pattern) {
      mode = (mode + 1) % TOTAL_MODE_SIZE;
      change_pattern = false;
    }
  } else if (!rightswitch) {
    // Serial.print("Right Switch Turned off\n");
    // delay(DELAY); 
    change_pattern = true;
  } else if (BLACKSCREEN_MODE){
    // Serial.print("BLACKSCREEN_MODE\n");
    // delay(DELAY); 
    mode = BLACKSCREEN;
  } else {
    // Serial.print("Default mode\n");
    // delay(DELAY); 
    mode = 3;
  }

  PaletteFillIn(motionIndex, pitch_gesture);      
  FastLED.show();
  FastLED.delay(1000 / updates_per_second[mode]);
}

void PosPrinter(int pos){
  // pos = 1: wave left
  // pos = 2: wave right
  // pos = 3: rotate left
  // pos = 4: rotate right
  // todo: more
  if(pos==1){
      Serial.print("wave left!");
      Serial.print("\n");
  }
  else if(pos==2){
      Serial.print("wave right!");
      Serial.print("\n");
  }
}

void WriteMPUReg(int nReg, unsigned char nVal) { // write something to MPU6050
  Wire.beginTransmission(MPU);
  Wire.write(nReg);
  Wire.write(nVal);
  Wire.endTransmission(true);
}

//从MPU6050读出一个字节的数据
//指定寄存器地址，返回读出的值
unsigned char ReadMPUReg(int nReg) {
  Wire.beginTransmission(MPU);
  Wire.write(nReg);
  Wire.requestFrom(MPU, 1, true);
  Wire.endTransmission(true);
  return Wire.read();
}

//从MPU6050读出加速度计三个分量、温度和三个角速度计
//保存在指定的数组中
void ReadAccGyr(int *pVals) {
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.requestFrom(MPU, nValCnt * 2, true);
  Wire.endTransmission(true);
  for (long i = 0; i < nValCnt; ++i) {
    pVals[i] = Wire.read() << 8 | Wire.read();
  }
}

//对大量读数进行统计，校准平均偏移量
void Calibration()
{
  float valSums[7] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0};
  //先求和
  for (int i = 0; i < nCalibTimes; ++i) {
    int mpuVals[nValCnt];
    ReadAccGyr(mpuVals);
    for (int j = 0; j < nValCnt; ++j) {
      valSums[j] += mpuVals[j];
    }
  }
  //再求平均
  for (int i = 0; i < nValCnt; ++i) {
    calibData[i] = int(valSums[i] / nCalibTimes);
  }
  calibData[2] += 16384; //设芯片Z轴竖直向下，设定静态工作点。
}

//算得Roll角。算法见文档。
float GetRoll(float *pRealVals, float fNorm) {
  float fNormXZ = sqrt(pRealVals[0] * pRealVals[0] + pRealVals[2] * pRealVals[2]);
  float fCos = fNormXZ / fNorm;
  return acos(fCos) * fRad2Deg;
}

//算得Pitch角。算法见文档。
float GetPitch(float *pRealVals, float fNorm) {
  float fNormYZ = sqrt(pRealVals[1] * pRealVals[1] + pRealVals[2] * pRealVals[2]);
  float fCos = fNormYZ / fNorm;
  return acos(fCos) * fRad2Deg;
}

//对读数进行纠正，消除偏移，并转换为物理量。公式见文档。
void Rectify(int *pReadout, float *pRealVals) {
  for (int i = 0; i < 3; ++i) {
    pRealVals[i] = (float)(pReadout[i] - calibData[i]) / 16384.0f;
  }
  pRealVals[3] = pReadout[3] / 340.0f + 36.53;
  for (int i = 4; i < 7; ++i) {
    pRealVals[i] = (float)(pReadout[i] - calibData[i]) / 131.0f;
  }
}

int CheckPitch(){
  // check if rotate left or right
  int checkres[2] = {0,0};
  int check_ptr = 0;
  for(int i = 0; i < QUEUE_LEN; i++){
      int ptr = (cur_ptr+1+i)%QUEUE_LEN;
      if(pitch_queue[ptr]>LEFT_PITCH_BAR){
        checkres[check_ptr] = 1;
        check_ptr ++;
        if(check_ptr==2){
          break;
        }
      }
      else if(pitch_queue[ptr]<RIGHT_PITCH_BAR){
        checkres[check_ptr] = -1;
        check_ptr ++;
        if(check_ptr==2){
          break;
        }       
      }
  }
  if(checkres[0]==-1 & checkres[1]==1){
    clearpitch();
    return 2; // wave right
  }
  if(checkres[0]==1 & checkres[1]==-1){
    clearpitch();
    return 1; // wave left
  }
  return 0;
}

void clearpitch(){
  for(int i = 0; i < QUEUE_LEN; i++){
    pitch_queue[i] = 0;
  }
}

uint8_t GetLEDIndex(uint8_t row, uint8_t col) {
  return row * 12 + col;
}

void PaletteFillIn(uint8_t motionIndex, int pitch_gesture)
{
  // uint8_t secondHand = (millis() / 1000) % 80;
  // static uint8_t lastSecond = 99;
  if (mode == DEFAULT_LED) // LED light Strips
  {
    for( int i = 0; i < NUM_LEDS; i++) {
      leds[i] = ColorFromPalette( RainbowStripeColors_p, motionIndex, BRIGHTNESS, LINEARBLEND);
      motionIndex += 3;
    }
  }
  else if (mode == RAIN_DROP) // raindrop
  {
    for (uint8_t i = 0; i < 6; i ++) {
      if (random8() % 20 == 0) { // change the number here to change the rain size
        NextRainRow[i] = true;
      } else {
        NextRainRow[i] = false;
      }
      RainPattern[GetLEDIndex(i, 11)] = RainPattern[GetLEDIndex(i, 11)] == RainPaletteSize - 1 ? 0 : RainPattern[GetLEDIndex(i, 11)];
      for (uint8_t j = 11; j > 0; j --) {
        uint8_t currentIndex = GetLEDIndex(i, j);
        if (RainPattern[currentIndex - 1] != 0) {
          RainPattern[currentIndex] = RainPattern[currentIndex - 1];
          RainPattern[currentIndex - 1] = (RainPattern[currentIndex - 1] + 1) % RainPaletteSize;
        }
        if (i % 2 == 0) {
          leds[GetLEDIndex(i, j)] = RainPalette[RainPattern[currentIndex]];
        } else {
          leds[GetLEDIndex(i, 11-j)] = RainPalette[RainPattern[currentIndex]];
        }
      }
      RainPattern[GetLEDIndex(i, 0)] = NextRainRow[i] ? 1 : RainPattern[GetLEDIndex(i, 0)];
      if (i % 2 == 0) {
        leds[GetLEDIndex(i, 0)] = RainPalette[RainPattern[GetLEDIndex(i, 0)]];
      } else {
        leds[GetLEDIndex(i, 11)] = RainPalette[RainPattern[GetLEDIndex(i, 0)]];
      }
    }
  }
  else if (mode == ARROW) // arrow
  {
    uint8_t currentOffset = motionIndex % 12;
    for (uint8_t i = 0; i < 6; i ++) {
      for (uint8_t j = 0; j < 12; j ++) {
        if (i % 2 == 0) {
          leds[GetLEDIndex(i, j)] = ArrowPalette[ArrowPattern[GetLEDIndex(i, (j+currentOffset) % 12)]];
        } else {
          leds[GetLEDIndex(i, 11-j)] = ArrowPalette[ArrowPattern[GetLEDIndex(i, (j+currentOffset) % 12)]];
        }
      }
    }        
  }
  else if(mode == HEART) // heart, wave left for big heart, wave right for small heart
  {        
    uint8_t currentOffset = pitch_gesture % 2; // pitch_gesture == 0 ? motionIndex % 2 : pitch_gesture % 2;
    for (uint8_t i = 0; i < 6; i ++) {
      for (uint8_t j = 0; j < 12; j ++) {
        if (i % 2 == 0) {
          leds[GetLEDIndex(5-i, j)] = HeartPalette[HorizontalHeartPattern[currentOffset][GetLEDIndex(i, j)]];
        } else {
          leds[GetLEDIndex(5-i, 11-j)] = HeartPalette[HorizontalHeartPattern[currentOffset][GetLEDIndex(i, j)]];
        }
      }
    }
  }
  else if (mode == GIXSOLID) // GIX Yellow
  {
    for (uint8_t i = 0; i < 6; i ++) {
      for (uint8_t j = 0; j < 12; j ++) {
        if (i % 2 == 0) {
          leds[GetLEDIndex(5-i, 11-j)] = GIXPalette[GIXPattern[GetLEDIndex(i, j)]];
        } else {
          leds[GetLEDIndex(5-i, j)] = GIXPalette[GIXPattern[GetLEDIndex(i, j)]];
        }
      }
    } 
  }
  else if (mode == GIXCOLOR) // GIX colors
  {
    for (uint8_t i = 0; i < 6; i ++) {
      for (uint8_t j = 0; j < 12; j ++) {
        if (i % 2 == 0) {
          leds[GetLEDIndex(5-i, 11-j)] = GIXPattern[GetLEDIndex(i, j)] == 1 ? ColorFromPalette( RainbowStripeColors_p, motionIndex, BRIGHTNESS, LINEARBLEND) : CRGB::Black;
        } else {
          leds[GetLEDIndex(5-i, j)] = GIXPattern[GetLEDIndex(i, j)] == 1 ? ColorFromPalette( RainbowStripeColors_p, motionIndex, BRIGHTNESS, LINEARBLEND) : CRGB::Black;
        }
        motionIndex += 3;
      }
    } 
  } 
  else if (mode == WHITESCREEN)
  {
    for (uint8_t i = 0; i < 6; i ++) {
      for (uint8_t j = 0; j < 12; j ++) {
        leds[GetLEDIndex(i, j)] = CRGB::Yellow;
      }
    }  
  } 
  else 
  {
    for (uint8_t i = 0; i < 6; i ++) {
      for (uint8_t j = 0; j < 12; j ++) {
        leds[GetLEDIndex(i, j)] = CRGB::Black;
      }
    } 
  }
}
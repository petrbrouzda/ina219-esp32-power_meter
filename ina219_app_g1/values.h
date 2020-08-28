#ifndef X_VALUES_H
#define X_VALUES_H

class Values
{
  public:
    long lastMeteringTime = 0;
    
    double total_uWh = 0;
    long total_ms = 0;
    double total_uAh = 0;
    
    float outVoltage;
    float outPower;
    
    float shuntvoltage;
    float busvoltage;
    float current_mA;
    float power_mW;
    float source;
    float source_mW;
    float shunt_power_mW;
    float shunt_pomer;
    
    double energy_uWh;
    double avg_power;
    
    float maxCurrentMin = 0;
    float maxCurrentTotal = 0;
    
    float minVoltageTotal = 100;
    float maxVoltageTotal = -100;
    float minVoltageMin = 100;
    float maxVoltageMin = -100;
    
    long meteringTime = 0;  

    void doCompute( int coMerim, long interval_ms, long curTime );
    void resetCounters();
    void resetMinuteData();
};

#endif

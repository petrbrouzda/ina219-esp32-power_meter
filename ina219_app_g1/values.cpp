#include "values.h"
#include <arduino.h>


void Values::doCompute( int coMerim, long interval_ms, long curTime, float lowHighThreshold )
{
  this->source = this->busvoltage + (this->shuntvoltage / 1000.0);
  this->source_mW = this->source * this->current_mA;

  if( coMerim==11 ) {
    this->outVoltage = this->busvoltage;
    this->outPower = this->power_mW;
  } else {
    this->outVoltage = this->source;
    this->outPower = this->source_mW;
  }

  this->shunt_power_mW = this->current_mA * this->shuntvoltage / 1000.0;
  this->shunt_pomer = this->power_mW>0 ? (this->shunt_power_mW/this->power_mW*100.0) : 0;

  this->lastMeteringTime = curTime;

  double energy_uWh = this->outPower * ((double)interval_ms) / ((double)3600); 
  this->total_uWh += energy_uWh;
  this->total_ms += interval_ms;
  this->avg_power = this->total_uWh * 3600.0 / ((double)this->total_ms);
  double capa_uAh = this->current_mA * ((double)interval_ms) / ((double)3600); 
  this->total_uAh += capa_uAh ;

  if( this->current_mA > lowHighThreshold ) {
    if( this->currentPowerState!=2 ) {
      // zmena stavu low->high
      this->currentPowerState = 2;
      this->lastHighPowerEventStartMsec = curTime;
      this->highPowerEvents++;
    }
    highPowerTime += interval_ms;
    this->highPower_uWh += energy_uWh;
    this->highPower_uAh += capa_uAh ;
  } else {
    if( this->currentPowerState!=1 ) {
      // zmena stavu high->low
      this->currentPowerState = 1;
      this->lastHighPowerEventLengthMsec = curTime - this->lastHighPowerEventStartMsec;
      if( this->lastHighPowerEventLengthMsec > this->maxHighPowerEventLengthMsec ) {
        this->maxHighPowerEventLengthMsec = this->lastHighPowerEventLengthMsec;
      }
    }    
    lowPowerTime += interval_ms;
    this->lowPower_uWh += energy_uWh ;
    this->lowPower_uAh += capa_uAh ;
  }

  this->meteringTime += interval_ms;
  
  if( this->current_mA > this->maxCurrentMin ) {
    this->maxCurrentMin = this->current_mA;
  }
  if( this->current_mA > this->maxCurrentTotal ) {
    this->maxCurrentTotal = this->current_mA;
    this->maxCurrentTime = time(NULL);
  }
  if( this->outVoltage > this->maxVoltageTotal ) {
    this->maxVoltageTotal = this->outVoltage;
    this->maxVoltageTime = time(NULL);
  }
  if( this->outVoltage > this->maxVoltageMin ) {
    this->maxVoltageMin = this->outVoltage;
  }
  if( this->outVoltage < this->minVoltageTotal ) {
    this->minVoltageTotal = this->outVoltage;
    this->minVoltageTime = time(NULL);
  }
  if( this->outVoltage < this->minVoltageMin ) {
    this->minVoltageMin = this->outVoltage;
  }
}

void Values::resetCounters()
{
  this->total_uWh = 0;
  this->total_ms = 0;
  this->total_uAh = 0;    
  this->maxCurrentTotal = 0;
  this->meteringTime = 0;
  this->minVoltageTotal = 100;
  this->maxVoltageTotal = -100;
  this->lowPowerTime = 0;
  this->highPowerTime = 0;
  this->lowPower_uAh = 0;
  this->highPower_uAh = 0;
  this->highPower_uWh = 0;
  this->lowPower_uWh = 0;
  this->currentPowerState = 1;
  this->highPowerEvents = 0;
  this->lastHighPowerEventLengthMsec = 0;
  this->lastHighPowerEventStartMsec = millis();
  this->maxHighPowerEventLengthMsec = 0;
  
  this->resetMinuteData();
}

void Values::resetMinuteData()
{
  this->maxCurrentMin = this->current_mA;
  this->minVoltageMin = this->outVoltage;
  this->maxVoltageMin = this->outVoltage;
  this->minVoltageTime = time(NULL);
  this->maxVoltageTime = time(NULL);
  this->maxCurrentTime = time(NULL);
}

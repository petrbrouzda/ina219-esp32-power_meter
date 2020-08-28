#include "values.h"

void Values::doCompute( int coMerim, long interval_ms, long curTime )
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

  this->energy_uWh = this->outPower * ((double)interval_ms) / ((double)3600); 
  this->total_uWh += this->energy_uWh;
  this->total_ms += interval_ms;
  this->avg_power = this->total_uWh * 3600.0 / ((double)this->total_ms);

  double capa_uAh = this->current_mA * ((double)interval_ms) / ((double)3600); 
  this->total_uAh += capa_uAh;

  this->meteringTime += interval_ms;

  
  
  if( this->current_mA > this->maxCurrentMin ) {
    this->maxCurrentMin = this->current_mA;
  }
  if( this->current_mA > this->maxCurrentTotal ) {
    this->maxCurrentTotal = this->current_mA;
  }

  if( this->outVoltage > this->maxVoltageTotal ) {
    this->maxVoltageTotal = this->outVoltage;
  }
  if( this->outVoltage > this->maxVoltageMin ) {
    this->maxVoltageMin = this->outVoltage;
  }
  if( this->outVoltage < this->minVoltageTotal ) {
    this->minVoltageTotal = this->outVoltage;
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
    this->maxCurrentMin = 0;
    this->maxCurrentTotal = 0;
    this->meteringTime = 0;
    this->minVoltageTotal = 100;
    this->maxVoltageTotal = -100;
    this->minVoltageMin = 100;
    this->maxVoltageMin = -100;
}

void Values::resetMinuteData()
{
    this->maxCurrentMin = this->current_mA;
    this->minVoltageMin = this->outVoltage;
    this->maxVoltageMin = this->outVoltage;
}

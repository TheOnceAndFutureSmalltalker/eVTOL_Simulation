#pragma once


// Recieves a notification of timestep update
class ISimulationAgent
{
public:
	virtual void begin() = 0;

	virtual void timestep_update(unsigned long long prev_time, unsigned long long cur_time) = 0;
};


// A device that can be charged by a charger
class IChargeableDevice
{
public:
	virtual void addCharge(double kWh) = 0;
	virtual double chargeRate() = 0;  // in kWh per second
	virtual bool hasFullCharge() = 0;
};


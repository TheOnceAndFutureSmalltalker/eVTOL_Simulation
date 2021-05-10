#ifndef SIM_TYPES
#define SIM_TYPES


// Any object that participates in a simulation by receiving timestep updates
// so it can update its state.
class SimulationAgent
{
public:
	// called as an initialization befomre any timestep updates
	virtual void begin() = 0;

	// called for each timestep and agen updates its state
	virtual void timestepUpdate(size_t prev_time, size_t cur_time) = 0;
};


// A device that can be charged at a charging station
class ChargeableDevice
{
public:
	// adds charge to the device in units of kWh
	virtual void addCharge(double kWh) = 0;

	// the kWh per millisecond that the device can receive charge 
	virtual double chargeRate() = 0;  

	// returns true if device is fully charged, false otherwise
	virtual bool hasFullCharge() = 0;
};

#endif  // SIM_TYPES
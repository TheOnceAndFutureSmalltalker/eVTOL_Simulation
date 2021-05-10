#ifndef EVTOL_SIMULATION
#define EVTOL_SIMULATION


#include <iostream>
#include <iomanip>
#include <vector>
#include <set>
#include <algorithm>
#include <numeric>
#include <stdexcept>

#include "evtol.h"
#include "evtol_factory.h"
#include "sim_timer.h"
#include "charge_station.h"


// basic simulation parameters
// TODO:  these could go in a config file or captured as command line args
// As it stands, modifying the parameters of the simulation requires a recompile.
#define TOTAL_NUMBER_EVTOLS 20
#define MAX_NUMBER_CHARGING_STALLS 3
#define TOTAL_MINUTES_SIMULATION_TIME 180
#define SIMULATION_TIME_COMPRESSION 60
#define TIMESTEP_IN_MILLISECONDS 1000

// These are the eVTOL configurations specified in the problem sheet.
// TODO:  this should definitely go in a config file!!
eVTOLConfiguration alpha_config("Alpha Company", 120, 320, 0.60, 1.6, 4, 0.25);
eVTOLConfiguration beta_config("Beta Company", 100, 100, 0.20, 1.5, 5, 0.10);
eVTOLConfiguration charlie_config("Charlie Company", 220, 320, 0.80, 2.2, 3, 0.05);
eVTOLConfiguration delta_config("Delta Company", 90, 120, 0.62, 0.8, 2, 0.22);
eVTOLConfiguration echo_config("Echo Company", 30, 150, 0.30, 5.8, 2, 0.61);



// This is a simulation specifically implementing the Joby eVTOL Simulation problem.
// Various parameters and configurations of this simulation can be modified.
// A different simulation, capturing different information, would require a different implementation.
// Several general simulation components are used however.
class eVTOLSimulation
{
public:
	eVTOLSimulation()
	{
		has_already_run_ = false;
		charging_station_ = nullptr;
	}

	// constructs and runs the entire simulation
	void run()
	{
		if (has_already_run_) throw std::logic_error("Each instance of eVTOLSimulation can only run once.");
		has_already_run_ = true;

		// create the charging station
		charging_station_ = new ChargingStation(MAX_NUMBER_CHARGING_STALLS);

		// create the eVTOL factory and populate it with the eVTOL prototypes
		eVTOLFactory factory;
		factory.addPrototype(eVTOL(alpha_config, charging_station_));
		factory.addPrototype(eVTOL(beta_config, charging_station_));
		factory.addPrototype(eVTOL(charlie_config, charging_station_));
		factory.addPrototype(eVTOL(delta_config, charging_station_));
		factory.addPrototype(eVTOL(echo_config, charging_station_));

		// create the population eVTOLs and make them agents of the simulation 
		int num_evtols = TOTAL_NUMBER_EVTOLS;
		while (num_evtols-- > 0)
		{
			eVTOL* evtol = factory.create_eVTOL();
			evtol->begin();
			evtols_.push_back(evtol);
		}
		charging_station_->begin();

		// create the timer and timestep event handler
		auto timestep_handler = [this](size_t prev_time, size_t cur_time) {
			// simply forward the timestep event to each of the simulation agents - evtols and charging station
			std::for_each(evtols_.begin(), evtols_.end(), [prev_time, cur_time](eVTOL* evtol) {
				evtol->timestepUpdate(prev_time, cur_time);
				});
			charging_station_->timestepUpdate(prev_time, cur_time);
			// print dot every second in real time for user feedback
			if (((cur_time / 1000) % SIMULATION_TIME_COMPRESSION) == 0) 
			{
				std::cout << ".";
				std::cout.flush();
			}
		};
		SimulationEventTimer timer(TIMESTEP_IN_MILLISECONDS, timestep_handler, TOTAL_MINUTES_SIMULATION_TIME, SIMULATION_TIME_COMPRESSION);

		// start the simulation
		std::cout << std::endl << "Starting Simulation" << std::endl;
		std::cout << std::fixed << std::setprecision(2);
		std::cout << "This will take approximately " << timer.totalSimulationTimeInRealMinutes() << " minutes." << std::endl;
		timer.start();
		std::cout << std::endl << "Simulation Finished" << std::endl;
	}


	void printResults()
	{
		using namespace std;
		cout << endl << endl << "******************************** R E S U L T S ********************************" << endl;
		printSimulationParameters();
		printIndividualVTOLResults();
		printCompanyGroupResults();
		cout << endl;
	}

	void printSimulationParameters()
	{
		using namespace std;
		cout << endl << "Simulation Parameters" << endl;
		cout << "  Number of eVTOLS:            " << TOTAL_NUMBER_EVTOLS << endl;
		cout << "  Number of Charging Bays:     " << MAX_NUMBER_CHARGING_STALLS << endl;
		cout << "  Total Simulation Time:       " << TOTAL_MINUTES_SIMULATION_TIME << " minutes" << endl;
		cout << "  Simulation Time Compression: " << SIMULATION_TIME_COMPRESSION << endl;
		cout << "  Timestep Interval:           " << TIMESTEP_IN_MILLISECONDS << " milliseconds" << endl;

	}

	void printIndividualVTOLResults()
	{
		using namespace std;
		cout << endl << "Individual eVTOL Stats" << endl;
		cout << setw(20) << "COMPANY" << setw(10) << "FLIGHT" << setw(10) << "CHARGE" << setw(10) << "WAIT" << setw(10) << "ENDING" << setw(11) << "CHARGE" << endl;
		cout << setw(20) << "" << setw(10) << "TIME" << setw(10) << "TIME" << setw(10) << "TIME" << setw(10) << "STATE" << setw(11) << "REMAINING" << endl;
		cout << setw(20) << "------------------" << setw(10) << "--------" << setw(10) << "--------" << setw(10) << "--------" << setw(10) << "--------" << setw(11) << "---------" << endl;
		std::for_each(evtols_.begin(), evtols_.end(), [](eVTOL* evtol) {
			cout << setw(20) << evtol->company_name();
			cout << fixed << setprecision(2);
			cout << setw(10) << (evtol->total_flight_time() / (1000.0 * 60));
			cout << setw(10) << (evtol->total_charge_time() / (1000.0 * 60));
			cout << setw(10) << (evtol->total_wait_time() / (1000.0 * 60));
			cout << setw(10) << evtol->stateName();
			cout << setw(10) << evtol->percentChargeRemaining() << "%" << endl;
			});
	}

	void printCompanyGroupResults()
	{
		using namespace std;
		cout << endl << "Company Stats" << endl;
		cout << setw(20) << "COMPANY" << setw(10) << "COUNT" << setw(10) << "AVERAGE" << setw(10) << "AVERAGE" << setw(10) << "AVERAGE" << setw(10) << "MAX" << setw(10) << "TOTAL" << endl;
		cout << setw(20) << "" << setw(10) << "" << setw(10) << "FLT TIME" << setw(10) << "CHG TIME" << setw(10) << "WAT TIME" << setw(10) << "NUMBER" << setw(10) << "PASSENGR" << endl;
		cout << setw(20) << "" << setw(10) << "" << setw(10) << "MINUTES" << setw(10) << "MINUTES" << setw(10) << "MINUTES" << setw(10) << "FAULTS" << setw(10) << "MILES" << endl;
		cout << setw(20) << "------------------" << setw(10) << "--------" << setw(10) << "--------" << setw(10) << "--------" << setw(10) << "--------" << setw(10) << "--------" << setw(10) << "--------" << endl;

		// get a list of companies from evtols list
		std::set<std::string> companies;
		for (auto evtol : evtols_)
		{
			companies.insert(evtol->company_name());
		}

		// for each company, calculate stats and print them out in one row
		for (std::string company : companies)
		{
			// get all vtols for current company
			std::vector<eVTOL*> company_evtols;
			std::copy_if(evtols_.begin(), evtols_.end(), std::back_inserter(company_evtols), [company](eVTOL* evtol) {return evtol->company_name() == company; });

			// calculate average flight time in minutes
			double avg_flight_time_minutes = std::accumulate(company_evtols.begin(), company_evtols.end(), 0.0, [](double total, eVTOL* evtol) { return total + evtol->total_flight_time() / (1000.0 * 60); });
			avg_flight_time_minutes /= company_evtols.size();

			// calculate average charge time in minutes
			double avg_charge_time_minutes = std::accumulate(company_evtols.begin(), company_evtols.end(), 0.0, [](double total, eVTOL* evtol) { return total + evtol->total_charge_time() / (1000.0 * 60); });
			avg_charge_time_minutes /= company_evtols.size();

			// calculate averate wait time in minutes
			double avg_wait_time_minutes = std::accumulate(company_evtols.begin(), company_evtols.end(), 0.0, [](double total, eVTOL* evtol) { return total + evtol->total_wait_time() / (1000.0 * 60); });
			avg_wait_time_minutes /= company_evtols.size();

			// calculate max faults
			std::vector<size_t> faults;
			std::transform(company_evtols.begin(), company_evtols.end(), std::back_inserter(faults), [](eVTOL* evtol) { return evtol->number_of_faults(); });
			auto max_faults_itr = std::max_element(faults.begin(), faults.end());
			size_t max_faults = *max_faults_itr;

			// calculate total passenger miles
			size_t total_passenger_miles = std::accumulate(company_evtols.begin(), company_evtols.end(), 0, [](double total, eVTOL* evtol) {
				return total + (evtol->total_flight_time() / (1000.0 * 60 * 60) * evtol->cruise_speed() * evtol->passenger_count());
				});

			// print out the current company's stats
			cout << setw(20) << company;
			cout << setw(10) << company_evtols.size();
			cout << fixed << setprecision(2);
			cout << setw(10) << avg_flight_time_minutes;
			cout << setw(10) << avg_charge_time_minutes;
			cout << setw(10) << avg_wait_time_minutes;
			cout << setw(10) << max_faults;
			cout << setw(10) << total_passenger_miles;
			cout << endl;
		}
	}


	~eVTOLSimulation()
	{
		// destroy all of the heap allocated simulation agents - the evtols and charging station
		std::for_each(evtols_.begin(), evtols_.end(), [](eVTOL* evtol) {delete evtol; });
		delete charging_station_;
	}

private:
	std::vector<eVTOL*> evtols_;
	ChargingStation* charging_station_;
	bool has_already_run_;
};

#endif  // EVTOL_SIMULATION
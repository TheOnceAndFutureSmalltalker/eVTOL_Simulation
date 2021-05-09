

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

// These are the eVTOL configurations.
// TODO:  this should definitely go in a config file!!
eVTOLConfiguration alpha_config("Alpha Company",     120, 320, 0.60, 1.6, 4, 0.25);
eVTOLConfiguration beta_config("Beta Company",       100, 100, 0.20, 1.5, 5, 0.10);
eVTOLConfiguration charlie_config("Charlie Company", 220, 320, 0.80, 2.2, 3, 0.05);
eVTOLConfiguration delta_config("Delta Company",      90, 120, 0.62, 0.8, 2, 0.22);
eVTOLConfiguration echo_config("Echo Company",        30, 150, 0.30, 5.8, 2, 0.61);



// This is a simulation specifically implementing the Joby eVTOL Simulation problem.
// Various parameters and configurations of this simulation can be modified.
// A different simulation, capturing different information, would require a different implementation.
// Several general simulation components are used however.
class eVTOLSimulation
{
public:
	eVTOLSimulation() 
	{ 
		has_already_run = false; 
		charging_station = nullptr;
	}

	// constructs and runs the entire simulation
	void run()
	{
		if(has_already_run) throw std::logic_error("Each instance of eVTOLSimulation can only run once.");
		has_already_run = true;

		// create the charging station
		charging_station = new ChargingStation(MAX_NUMBER_CHARGING_STALLS);
		
		// create the eVTOL factory and populate it with the eVTOL prototypes
		eVTOLFactory factory;
		factory.add_prototype(eVTOL(alpha_config, charging_station));
		factory.add_prototype(eVTOL(beta_config, charging_station));
		factory.add_prototype(eVTOL(charlie_config, charging_station));
		factory.add_prototype(eVTOL(delta_config, charging_station));
		factory.add_prototype(eVTOL(echo_config, charging_station));

		// create the population eVTOLs and make them agents of the simulation 
		int num_evtols = TOTAL_NUMBER_EVTOLS;
		while (num_evtols-- > 0)
		{
			eVTOL* evtol = factory.create_eVTOL();
			evtol->begin();
			evtols.push_back(evtol);
		}

		// create the timer and timestep event handler
		auto timestep_handler = [this](size_t prev_time, size_t cur_time) {
			// simply forward the timestep event to each of the simulation agents - evtols and charging station
			std::for_each(evtols.begin(), evtols.end(), [prev_time, cur_time](eVTOL* evtol) {
				evtol->timestep_update(prev_time, cur_time);
				});
			charging_station->timestep_update(prev_time, cur_time);
			// print dot every second in real time for user feedback
			if (((cur_time / 1000) % SIMULATION_TIME_COMPRESSION) == 0) std::cout << "."; 
		};
		SimulationEventTimer timer(TIMESTEP_IN_MILLISECONDS, timestep_handler, TOTAL_MINUTES_SIMULATION_TIME, SIMULATION_TIME_COMPRESSION);

		// start the simulation
		std::cout << std::endl << "Starting Simulation" << std::endl;
		timer.start();
		std::cout << std::endl << "Simulation Finished" << std::endl;
	}


	void print_results()
	{
		using namespace std;
		cout << endl << endl << "******************************** R E S U L T S ********************************" << endl;
		print_individual_results();
		print_company_results();
	}

	void print_individual_results()
	{
		using namespace std;
		cout << endl << "Individual eVTOL Stats" << endl;
		cout << setw(20) << "COMPANY" << setw(10) << "FLIGHT" << setw(10) << "CHARGE" << setw(10) << "WAIT" << setw(11) << "CHARGE" << endl;
		cout << setw(20) << "" << setw(10) << "TIME" << setw(10) << "TIME" << setw(10) << "TIME" << setw(11) << "REMAINING" << endl;
		cout << setw(20) << "------------------" << setw(10) << "--------" << setw(10) << "--------" << setw(10) << "--------" << setw(11) << "---------" << endl;
		std::for_each(evtols.begin(), evtols.end(), [](eVTOL* evtol) {
			cout << setw(20) << evtol->get_company_name();
			cout << fixed << setprecision(2);
			cout << setw(10) << (evtol->get_total_flight_time() / (1000.0 * 60));
			cout << setw(10) << (evtol->get_total_charge_time() / (1000.0 * 60));
			cout << setw(10) << (evtol->get_total_wait_time() / (1000.0 * 60));
			cout << setw(10) << evtol->percent_charge_remaining() << "%" << endl;
			});
	}

	void print_company_results()
	{
		using namespace std;
		cout << endl << "Company Stats" << endl;
		cout << setw(20) << "COMPANY" << setw(10) << "COUNT" << setw(10) << "AVERAGE" << setw(10) << "AVERAGE" << setw(10) << "AVERAGE" << setw(10) << "MAX" << setw(10) << "TOTAL" << endl;
		cout << setw(20) << "" << setw(10) << "" << setw(10) << "FLIGHT" << setw(10) << "CHARGE" << setw(10) << "WAIT" << setw(10) << "NUMBER" << setw(10) << "PASSENGR" << endl;
		cout << setw(20) << "" << setw(10) << "" << setw(10) << "TIME" << setw(10) << "TIME" << setw(10) << "TIME" << setw(10) << "FAULTS" << setw(10) << "MILES" << endl;
		cout << setw(20) << "------------------" << setw(10) << "--------" << setw(10) << "--------" << setw(10) << "--------" << setw(10) << "--------" << setw(10) << "--------" << setw(10) << "--------" << endl;
		
		// get a list of companies
		std::set<std::string> companies;
		for(auto evtol : evtols)
		{
			companies.insert(evtol->get_company_name());
		}

		// for each company, calculate stats and print them out in one row
		for (std::string company : companies)
		{
			// get all vtols for current company
			std::vector<eVTOL*> company_evtols;
			std::copy_if(evtols.begin(), evtols.end(), std::back_inserter(company_evtols), [company](eVTOL* evtol) {return evtol->get_company_name() == company; });
			
			// calculate average flight time in minutes
			double avg_flight_time_minutes = std::accumulate(company_evtols.begin(), company_evtols.end(), 0.0, [](double total, eVTOL* evtol) { return total + evtol->get_total_flight_time() / (1000.0 * 60); });
			avg_flight_time_minutes /= company_evtols.size();

			// calculate average charge time in minutes
			double avg_charge_time_minutes = std::accumulate(company_evtols.begin(), company_evtols.end(), 0.0, [](double total, eVTOL* evtol) { return total + evtol->get_total_charge_time() / (1000.0 * 60); });
			avg_charge_time_minutes /= company_evtols.size();

			// calculate averate wait time in minutes
			double avg_wait_time_minutes = std::accumulate(company_evtols.begin(), company_evtols.end(), 0.0, [](double total, eVTOL* evtol) { return total + evtol->get_total_wait_time() / (1000.0 * 60); });
			avg_wait_time_minutes /= company_evtols.size();

			// calculate max faults
			std::vector<double> faults;
			std::transform(company_evtols.begin(), company_evtols.end(), std::back_inserter(faults), [](eVTOL* evtol) { return evtol->get_faults(); });
			auto max_faults_itr = std::max_element(faults.begin(), faults.end());
			double max_faults = *max_faults_itr;
			
			// calculate total passenger miles
			size_t total_passenger_miles = std::accumulate(company_evtols.begin(), company_evtols.end(), 0, [](double total, eVTOL* evtol) { 
				return total + (evtol->get_total_flight_time() / (1000.0 * 60 * 60) * evtol->get_cruise_speed() * evtol->get_passenger_count());
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
		// destroy all of the heap allocated agents
		std::for_each(evtols.begin(), evtols.end(), [](eVTOL* evtol) {delete evtol; });
	}

private:
	std::vector<eVTOL*> evtols;
	ChargingStation* charging_station;
	bool has_already_run;
};



int main()
{
	eVTOLSimulation simulation;
	simulation.run();
	simulation.print_results();

	//test_eVTOL();
} 


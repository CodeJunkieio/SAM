static var_info _cm_vtab_generic_system[] = {
	// VARTYPE	DATATYPE	NAME	LABEL	UNITS	META	GROUP	REQUIRED_IF	CONSTRAINTS	UI_HINTS
{ 	SSC_INPUT, 	SSC_NUMBER, 	"spec_mode", 	"Spec mode: 0=constant CF,1=profile", 	"", 	"", 	"Power Plant", 	"*", 	"", 	""},
{ 	SSC_INPUT, 	SSC_NUMBER, 	"derate", 	"Derate", 	"%", 	"", 	"Power Plant", 	"*", 	"", 	""},
{ 	SSCINOUT, 	SSC_NUMBER, 	"system_capacity", 	"Nameplace Capcity", 	"kW", 	"", 	"Power Plant", 	"*", 	"", 	""},
{ 	SSC_INPUT, 	SSC_NUMBER, 	"user_capacity_factor", 	"Capacity Factor", 	"%", 	"", 	"Power Plant", 	"*", 	"", 	""},
{ 	SSC_INPUT, 	SSC_NUMBER, 	"heat_rate", 	"Heat Rate", 	"MMBTUs/MWhe", 	"", 	"Power Plant", 	"*", 	"", 	""},
{ 	SSC_INPUT, 	SSC_NUMBER, 	"conv_eff", 	"Conversion Efficiency", 	"%", 	"", 	"Power Plant", 	"*", 	"", 	""},
{ 	SSC_INPUT, 	SSC_ARRAY, 	"energy_output_array", 	"Array of Energy Output Profile", 	"kW", 	"", 	"Power Plant", 	"*", 	"", 	""},
{ 	SSC_INPUT, 	SSC_NUMBER, 	"system_use_lifetime_output", 	"Generic lifetime simulation", 	"0/1", 	"", 	"", 	"?=0", 	"INTEGER,MIN=0,MAX=1", 	""},
{ 	SSC_INPUT, 	SSC_NUMBER, 	"analysis_period", 	"Lifetime analysis period", 	"years", 	"", 	"", 	"system_use_lifetime_output=1", 	"", 	""},
{ 	SSC_INPUT, 	SSC_ARRAY, 	"generic_degradation", 	"Annual module degradation", 	"%/year", 	"", 	"", 	"system_use_lifetime_output=1", 	"", 	""},
{ 	SSC_OUTPUT, 	SSC_ARRAY, 	"monthly_energy", 	"Monthly Energy", 	"kWh", 	"", 	"", 	"*", 	"LENGTH=12", 	""},
{ 	SSC_OUTPUT, 	SSC_NUMBER, 	"annual_energy", 	"Annual Energy", 	"kWh", 	"", 	"", 	"*", 	"", 	""},
{ 	SSC_OUTPUT, 	SSC_NUMBER, 	"annual_fuel_usage", 	"Annual Fuel Usage", 	"kWht", 	"", 	"", 	"*", 	"", 	""},
{ 	SSC_OUTPUT, 	SSC_NUMBER, 	"water_usage", 	"Annual Water Usage", 	"", 	"", 	"", 	"*", 	"", 	""},
{ 	SSC_OUTPUT, 	SSC_NUMBER, 	"system_heat_rate", 	"Heat Rate Conversion Factor", 	"MMBTUs/MWhe", 	"", 	"", 	"*", 	"", 	""},
{ 	SSC_OUTPUT, 	SSC_NUMBER, 	"capacity_factor", 	"Capacity factor", 	"%", 	"", 	"", 	"*", 	"", 	""},
{ 	SSC_OUTPUT, 	SSC_NUMBER, 	"kwh_per_kw", 	"First year kWh/kW", 	"kWh/kW", 	"", 	"", 	"*", 	"", 	""},
{ 	SSC_INPUT, 	SSC_NUMBER, 	"dc_adjust:constant", 	"DC Constant loss adjustment", 	"%", 	"", 	"", 	"", 	"MAX=100", 	""},
{ 	SSC_INPUT, 	SSC_ARRAY, 	"dc_adjust:hourly", 	"DC Hourly loss adjustments", 	"%", 	"", 	"", 	"", 	"LENGTH=8760", 	""},
{ 	SSC_INPUT, 	SSC_MATRIX, 	"dc_adjust:periods", 	"DC Period-based loss adjustments", 	"%", 	"n x 3 matrix [ start, end, loss ]", 	"", 	"", 	"COLS=3", 	""},
{ 	SSC_INPUT, 	SSC_NUMBER, 	"adjust:constant", 	"Constant loss adjustment", 	"%", 	"", 	"", 	"*", 	"MAX=100", 	""},
{ 	SSC_INPUT, 	SSC_ARRAY, 	"adjust:hourly", 	"Hourly loss adjustments", 	"%", 	"", 	"", 	"?", 	"LENGTH=8760", 	""},
{ 	SSC_INPUT, 	SSC_MATRIX, 	"adjust:periods", 	"Period-based loss adjustments", 	"%", 	"n x 3 matrix [ start, end, loss ]", 	"", 	"?", 	"COLS=3", 	""},
{ 	SSC_OUTPUT, 	SSC_ARRAY, 	"gen", 	"System power generated", 	"kW", 	"", 	"", 	"*", 	"", 	""},
var_info_invalid};
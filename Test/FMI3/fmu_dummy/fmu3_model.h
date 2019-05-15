/*
Copyright (C) 2012 Modelon AB

This program is free software: you can redistribute it and/or modify
it under the terms of the BSD style license.

the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
FMILIB_License.txt file for more details.

You should have received a copy of the FMILIB_License.txt file
along with this program. If not, contact Modelon AB <http://www.modelon.com>.
*/

#ifndef FMU3_MODEL_H_
#define FMU3_MODEL_H_
#include <FMI3/fmi3Functions.h>

#include <fmu_dummy/fmu3_model_defines.h>
#ifndef FMI3_Export
	#define FMI3_Export DllExport
#endif
typedef struct {
	/*************** FMI ME 2.0 ****************/
	fmi3Real					states			[N_STATES];
	fmi3Real					states_nom		[N_STATES];
	fmi3Real					states_der		[N_STATES];
	fmi3Real					event_indicators[N_EVENT_INDICATORS];
	fmi3Real					reals			[N_REAL];
	fmi3Integer				integers		[N_INTEGER];
	fmi3Boolean				booleans		[N_BOOLEAN];
	fmi3String				strings			[N_STRING];

	/* fmiInstantiateModel */
	fmi3Boolean				loggingOn;
	char					instanceName	[BUFFER];
	char					GUID			[BUFFER];
	const fmi3CallbackFunctions*	functions;

	/* fmiSetTime */
	fmi3Real					fmitime;

	/* fmiInitializeModel */
	fmi3Boolean				toleranceControlled;
	fmi3Real					relativeTolerance;
	fmi3EventInfo			eventInfo;

	/*************** FMI CS 2.0. Depends on the ME fields above and functions ****************/
	fmi3Real					states_prev		[N_STATES];

	/* fmiInstantiateSlave */
	char					fmuLocation		[BUFFER];
	fmi3Boolean				visible;

	/* fmiInitializeSlave */
	fmi3Real					tStart;
	fmi3Boolean				StopTimeDefined;
	fmi3Real					tStop;

	/* fmiSetRealInputDerivatives */
	fmi3Real					input_real		[N_INPUT_REAL][N_INPUT_REAL_MAX_ORDER + 1];

	/* fmiGetRealOutputDerivatives */
	fmi3Real					output_real		[N_OUTPUT_REAL][N_OUTPUT_REAL_MAX_ORDER + 1];

} component_t;

typedef component_t* component_ptr_t;

/* FMI 2.0 Common Functions */
const char*		fmi_get_version();

fmi3Status		fmi_set_debug_logging(
													fmi3Component c,
													fmi3Boolean loggingOn);

fmi3Component fmi_instantiate (
    fmi3String instanceName,
    fmi3Type fmuType,
    fmi3String fmuGUID,
    fmi3String fmuLocation,
    const fmi3CallbackFunctions* functions,
    fmi3Boolean visible,
    fmi3Boolean loggingOn);

void fmi_free_instance(
    fmi3Component c);

fmi3Status fmi_setup_experiment(fmi3Component c,
    fmi3Boolean toleranceDefined, fmi3Real tolerance,
    fmi3Real startTime, fmi3Boolean stopTimeDefined,
    fmi3Real stopTime);
fmi3Status		fmi_enter_initialization_mode(fmi3Component c);
fmi3Status		fmi_exit_initialization_mode(fmi3Component c);

fmi3Status		fmi_terminate(fmi3Component c);

fmi3Status		fmi_reset(
													fmi3Component c);


fmi3Status		fmi_get_real(			
													fmi3Component c,
													const fmi3ValueReference vr[],
													size_t nvr, fmi3Real value[]);

fmi3Status		fmi_get_integer(	
													fmi3Component c,
													const fmi3ValueReference vr[],
													size_t nvr,
													fmi3Integer value[]);
fmi3Status		fmi_get_boolean(
													fmi3Component c,
													const fmi3ValueReference vr[],
													size_t nvr,
													fmi3Boolean value[]);

fmi3Status		fmi_get_string(
													fmi3Component c,
													const fmi3ValueReference vr[],
													size_t nvr,
													fmi3String  value[]);

fmi3Status		fmi_set_real(
													fmi3Component c,
													const fmi3ValueReference vr[],
													size_t nvr,
													const fmi3Real value[]);
fmi3Status		fmi_set_integer(
													fmi3Component c,
													const fmi3ValueReference vr[],
													size_t nvr,
													const fmi3Integer value[]);

fmi3Status		fmi_set_boolean(
													fmi3Component c,
													const fmi3ValueReference vr[],
													size_t nvr,
													const fmi3Boolean value[]);

fmi3Status		fmi_set_string(
													fmi3Component c,
													const fmi3ValueReference vr[],
													size_t nvr,
													const fmi3String  value[]);

/* FMI 2.0 ME Functions */
const char*		fmi_get_model_types_platform();

fmi3Status		fmi_enter_event_mode(fmi3Component c);
fmi3Status		fmi_new_discrete_states(fmi3Component c, fmi3EventInfo* eventInfo);
fmi3Status		fmi_enter_continuous_time_mode(fmi3Component c);

fmi3Status		fmi_set_time(
													fmi3Component c,
													fmi3Real fmitime);

fmi3Status		fmi_set_continuous_states(
													fmi3Component c,
													const fmi3Real x[],
													size_t nx);

fmi3Status fmi_completed_integrator_step(
    fmi3Component c,
    fmi3Boolean noSetFMUStatePriorToCurrentPoint,
    fmi3Boolean* enterEventMode, fmi3Boolean* terminateSimulation);

fmi3Status		fmi_get_derivatives(
													fmi3Component c,
													fmi3Real derivatives[],
													size_t nx);

fmi3Status		fmi_get_event_indicators(
													fmi3Component c,
													fmi3Real eventIndicators[],
													size_t ni);

fmi3Status		fmi_get_continuous_states(
													fmi3Component c,
													fmi3Real states[],
													size_t nx);

fmi3Status		fmi_get_nominals_of_continuousstates(	
													fmi3Component c,
													fmi3Real x_nominal[],
													size_t nx);


/* FMI 2.0 CS Functions */
#ifdef fmi3Functions_h

const char*		fmi_get_types_platform();

void			fmi_free_slave_instance(
													fmi3Component c);

fmi3Status		fmi_set_real_input_derivatives(
													fmi3Component c,
													const fmi3ValueReference vr[],
													size_t nvr,
													const fmi3Integer order[],
													const fmi3Real value[]);

fmi3Status		fmi_get_real_output_derivatives(
													fmi3Component c,
													const fmi3ValueReference vr[],
													size_t nvr,
													const fmi3Integer order[],
													fmi3Real value[]);

fmi3Status		fmi_cancel_step(
													fmi3Component c);
fmi3Status		fmi_do_step(
													fmi3Component c,
													fmi3Real currentCommunicationPoint,
													fmi3Real communicationStepSize,
													fmi3Boolean newStep);

fmi3Status		fmi_get_status(
													fmi3Component c,
													const fmi3StatusKind s,
													fmi3Status*  value);

fmi3Status		fmi_get_real_status(
													fmi3Component c,
													const fmi3StatusKind s,
													fmi3Real*    value);

fmi3Status		fmi_get_integer_status(
													fmi3Component c,
													const fmi3StatusKind s,
													fmi3Integer* value);

fmi3Status		fmi_get_boolean_status(
													fmi3Component c,
													const fmi3StatusKind s,
													fmi3Boolean* value);

fmi3Status		fmi_get_string_status(
													fmi3Component c,
													const fmi3StatusKind s,
													fmi3String*  value);

#endif /* End of fmi3Functions_h */
#endif /* End of header FMU3_MODEL_H_ */

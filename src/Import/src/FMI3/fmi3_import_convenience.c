/*
    Copyright (C) 2012 Modelon AB

    This program is free software: you can redistribute it and/or modify
    it under the terms of the BSD style license.

     This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    FMILIB_License.txt file for more details.

    You should have received a copy of the FMILIB_License.txt file
    along with this program. If not, contact Modelon AB <http://www.modelon.com>.
*/

#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <FMI3/fmi3_xml_model_description.h>
#include <FMI3/fmi3_functions.h>

#include "fmi3_import_impl.h"

/**
	\brief Collect model information by counting the number of variables with specific properties and fillinf in fmi3_import_model_counts_t struct.
	\param fmu - An fmu object as returned by fmi3_import_parse_xml().
	\param counts - a pointer to a preallocated struct.
*/
void fmi3_import_collect_model_counts(fmi3_import_t* fmu, fmi3_import_model_counts_t* counts) {
	jm_vector(jm_voidp)* vars = fmi3_xml_get_variables_original_order(fmu->md);
    size_t nv, i;
	memset(counts,0,sizeof(fmi3_import_model_counts_t));
	if(!vars) return;
    nv = jm_vector_get_size(jm_voidp)(vars);	
    for(i = 0; i< nv; i++) {
		fmi3_xml_variable_t* var = (fmi3_xml_variable_t*)jm_vector_get_item(jm_voidp)(vars, i); 
				switch (fmi3_xml_get_variability(var)) {
		case fmi3_variability_enu_constant:
			counts->num_constants++;
			break;
		case fmi3_variability_enu_fixed:
			counts->num_fixed++;
			break;
		case fmi3_variability_enu_tunable:
			counts->num_tunable++;
			break;
		case fmi3_variability_enu_discrete:
			counts->num_discrete++;
			break;
		case fmi3_variability_enu_continuous:
			counts->num_continuous++;
			break;
		default:
			assert(0);
		}
		switch(fmi3_xml_get_causality(var)) {
		case fmi3_causality_enu_parameter:
			counts->num_parameters++;
			break;
		case fmi3_causality_enu_calculated_parameter:
			counts->num_calculated_parameters++;
			break;
		case fmi3_causality_enu_input:
			counts->num_inputs++;
			break;
		case fmi3_causality_enu_output:
			counts->num_outputs++;
			break;
		case fmi3_causality_enu_local:
			counts->num_local++;
			break;
		case fmi3_causality_enu_independent:
			counts->num_local++;
			break;
		default: assert(0);
		}
		switch(fmi3_xml_get_variable_base_type(var)) {
		case fmi3_base_type_float64:
			counts->num_float64_vars++;
			break;
		case fmi3_base_type_float32:
			counts->num_float32_vars++;
			break;
		case fmi3_base_type_int:
			counts->num_integer_vars++;
			break;
		case fmi3_base_type_bool:
			counts->num_bool_vars++;
			break;
		case fmi3_base_type_str:
			counts->num_string_vars++;
			break;
		case fmi3_base_type_enum:
			counts->num_enum_vars++;
			break;
		default:
			assert(0);
		}
    }
    return;
}

/* Transform msgIn into msgOut by expanding variable references of the form #<Type><VR># into variable names
  and replacing '##' with a single # */
static void fmi3_import_expand_variable_references_impl(fmi3_import_t* fmu, const char* msgIn){
	jm_vector(char)* msgOut = &fmu->logMessageBufferExpanded;
	fmi3_xml_model_description_t* md = fmu->md;
	jm_callbacks* callbacks = fmu->callbacks;
    char curCh;
	const char* firstRef;
    size_t i; /* next char index after curCh in msgIn*/ 
	size_t msgLen = strlen(msgIn)+1; /* original message length including terminating 0 */

	if(jm_vector_reserve(char)(msgOut, msgLen + 100) < msgLen + 100) {
		jm_log(fmu->callbacks,"LOGGER", jm_log_level_warning, "Could not allocate memory for the log message");
		jm_vector_resize(char)(msgOut, 6);
		memcpy(jm_vector_get_itemp(char)(msgOut,0),"ERROR",6); /* at least 16 chars are always there */
		return;
	}

	/* check if there are any refs at all and copy the head of the string without references */
	firstRef = strchr(msgIn, '#');
	if(firstRef) {
		i = firstRef - msgIn;
		jm_vector_resize(char)(msgOut, i);
		if(i) {
			memcpy(jm_vector_get_itemp(char)(msgOut, 0), msgIn, i);
		}
		curCh = msgIn[i++];
	}
	else {
		jm_vector_resize(char)(msgOut, msgLen);
		memcpy(jm_vector_get_itemp(char)(msgOut, 0), msgIn, msgLen);
		return;
	}
    do {
        if (curCh!='#') {
            jm_vector_push_back(char)(msgOut, curCh); /* copy in to out */
        }
		else if(msgIn[i] == '#') {
			jm_vector_push_back(char)(msgOut, '#');
			i++; /* skip the second # */
		}
		else {
            unsigned int bufVR;
			fmi3_value_reference_t vr;
			char typeChar = msgIn[i++];
			size_t pastePos = jm_vector_get_size(char)(msgOut);

            /*
                TODO: remove everything associated with baseType, in this func because vrs will
                be globally unique: https://github.com/modelica/fmi-standard/issues/522
                - This function's doc. must also be changed
                - Removing real for now, and not adding anything for floatXX
            */
			fmi3_base_type_enu_t baseType; 

			size_t num_digits;
			fmi3_xml_variable_t* var;
			const char* name;
			size_t nameLen;
			switch(typeChar) {
				case 'i': 
					baseType = fmi3_base_type_int;
					break;
				case 'b': 
					baseType = fmi3_base_type_bool;
					break;
				case 's': 
					baseType = fmi3_base_type_str;
					break;
				default:
					jm_vector_push_back(char)(msgOut, 0);
					jm_log(callbacks,"LOGGER", jm_log_level_warning, 
						"Expected type specification character 'r', 'i', 'b' or 's' in log message here: '%s'", 
					jm_vector_get_itemp(char)(msgOut,0));
                    jm_vector_resize(char)(msgOut, msgLen);
					memcpy(jm_vector_get_itemp(char)(msgOut,0),msgIn,msgLen);
					return;
			}
            curCh = msgIn[i++];
			while( isdigit(curCh) ) {
				jm_vector_push_back(char)(msgOut, curCh);
	            curCh = msgIn[i++];
			}
			num_digits = jm_vector_get_size(char)(msgOut) - pastePos;
			jm_vector_push_back(char)(msgOut, 0);
			if(num_digits == 0) {
				jm_log(callbacks,"LOGGER", jm_log_level_warning, "Expected value reference in log message here: '%s'", jm_vector_get_itemp(char)(msgOut,0));
                                jm_vector_resize(char)(msgOut, msgLen);
                jm_vector_resize(char)(msgOut, msgLen);
				memcpy(jm_vector_get_itemp(char)(msgOut,0),msgIn,msgLen);
				return;
			}
			else if(curCh != '#') {
				jm_log(callbacks,"LOGGER", jm_log_level_warning, "Expected terminating '#' in log message here: '%s'", jm_vector_get_itemp(char)(msgOut,0));
                                jm_vector_resize(char)(msgOut, msgLen);
                jm_vector_resize(char)(msgOut, msgLen);
				memcpy(jm_vector_get_itemp(char)(msgOut,0),msgIn,msgLen);
				return;
			}
			
			if(sscanf(jm_vector_get_itemp(char)(msgOut, pastePos), "%u",&bufVR) != 1) {
				jm_log(callbacks,"LOGGER", jm_log_level_warning, "Could not decode value reference in log message here: '%s'", jm_vector_get_itemp(char)(msgOut,0));
                                jm_vector_resize(char)(msgOut, msgLen);
                jm_vector_resize(char)(msgOut, msgLen);
				memcpy(jm_vector_get_itemp(char)(msgOut,0),msgIn,msgLen);
				return;
			}
            vr = bufVR;
			var = fmi3_xml_get_variable_by_vr(md,baseType,vr);
			if(!var) {
				jm_log(callbacks,"LOGGER", jm_log_level_warning, "Could not find variable referenced in log message here: '%s'", jm_vector_get_itemp(char)(msgOut,0));
                                jm_vector_resize(char)(msgOut, msgLen);
                jm_vector_resize(char)(msgOut, msgLen);
				memcpy(jm_vector_get_itemp(char)(msgOut,0),msgIn,msgLen);
				return;
			}
			name = fmi3_xml_get_variable_name(var);
			nameLen = strlen(name);
			if(jm_vector_resize(char)(msgOut, pastePos + nameLen) != pastePos + nameLen) {
				jm_log(callbacks,"LOGGER", jm_log_level_warning, "Could not allocate memory for the log message");
                                jm_vector_resize(char)(msgOut, msgLen);
                jm_vector_resize(char)(msgOut, msgLen);
				memcpy(jm_vector_get_itemp(char)(msgOut,0),msgIn,msgLen);
				return;
			};
			memcpy(jm_vector_get_itemp(char)(msgOut, pastePos), name, nameLen);
        }
        curCh = msgIn[i++];
    } while (curCh);
    jm_vector_push_back(char)(msgOut, 0);
}

void fmi3_import_expand_variable_references(fmi3_import_t* fmu, const char* msgIn, char* msgOut, size_t maxMsgSize) {
	fmi3_import_expand_variable_references_impl(fmu, msgIn);
	strncpy(msgOut, jm_vector_get_itemp(char)(&fmu->logMessageBufferExpanded,0), maxMsgSize);
	msgOut[maxMsgSize - 1] = '\0';
}

void fmi3_log_forwarding(fmi3_component_environment_t c, fmi3_string_t instanceName, fmi3_status_t status, fmi3_string_t category, fmi3_string_t message, ...) {
    va_list args;
    va_start (args, message);
	fmi3_log_forwarding_v(c, instanceName, status, category, message, args);
    va_end (args);
}

void fmi3_log_forwarding_v(fmi3_component_environment_t c, fmi3_string_t instanceName, fmi3_status_t status, fmi3_string_t category, fmi3_string_t message, va_list args) {
#define BUFSIZE JM_MAX_ERROR_MESSAGE_SIZE
    char buffer[BUFSIZE], *buf, *curp, *msg;
	const char* statusStr;
	fmi3_import_t* fmu = (fmi3_import_t*)c;
	jm_callbacks* cb;
	jm_log_level_enu_t logLevel;

	if(fmu) {
		 cb = fmu->callbacks;
         buf = jm_vector_get_itemp(char)(&fmu->logMessageBufferCoded,0);
	}
	else  {
		cb = jm_get_default_callbacks();
        buf = buffer;
    }
	logLevel = cb->log_level;
	switch(status) {
		case fmi3_status_discard:
		case fmi3_status_pending:
		case fmi3_status_ok:
			logLevel = jm_log_level_info;
			break;
		case fmi3_status_warning:
			logLevel = jm_log_level_warning;
			break;
		case fmi3_status_error:
			logLevel = jm_log_level_error;
			break;
		case fmi3_status_fatal:
		default:
			logLevel = jm_log_level_fatal;
	}

    if(logLevel > cb->log_level) return;

	curp = buf;
    *curp = 0;

	if(category) {
        curp += jm_snprintf(curp, 100, "[%s]", category);        
    }
	statusStr = fmi3_status_to_string(status);
    curp += jm_snprintf(curp, 200, "[FMU status:%s] ", statusStr);        	

	if(fmu) {
	    int bufsize = jm_vector_get_size(char)(&fmu->logMessageBufferCoded);
        int len;
#ifdef JM_VA_COPY
        va_list argscp;
        JM_VA_COPY(argscp, args);
#endif
        len = jm_vsnprintf(curp, bufsize -(curp-buf), message, args);
        if(len > (bufsize -(curp-buf+1))) {
            int offset = (curp-buf);
            len = jm_vector_resize(char)(&fmu->logMessageBufferCoded, len + offset + 1) - offset;
            buf = jm_vector_get_itemp(char)(&fmu->logMessageBufferCoded,0);
            curp = buf + offset;
#ifdef JM_VA_COPY
            jm_vsnprintf(curp, len, message, argscp);
#endif
        }
#ifdef JM_VA_COPY
        va_end(argscp);
#endif
		fmi3_import_expand_variable_references(fmu, buf, cb->errMessageBuffer,JM_MAX_ERROR_MESSAGE_SIZE);
		msg = jm_vector_get_itemp(char)(&fmu->logMessageBufferExpanded,0);
	}
	else {
        jm_vsnprintf(curp, BUFSIZE -(curp-buf), message, args);
		strncpy(cb->errMessageBuffer, buf, JM_MAX_ERROR_MESSAGE_SIZE);
		cb->errMessageBuffer[JM_MAX_ERROR_MESSAGE_SIZE - 1] = '\0';
		msg = cb->errMessageBuffer;
	}
	if(cb->logger) {
		cb->logger(cb, instanceName, logLevel, msg);
	}

}

void  fmi3_default_callback_logger(fmi3_component_environment_t c, fmi3_string_t instanceName, fmi3_status_t status, fmi3_string_t category, fmi3_string_t message, ...) {
    va_list args;
    char buf[BUFSIZE], *curp;
    va_start (args, message);
    curp = buf;
    *curp = 0;
    if(instanceName) {
        curp += jm_snprintf(curp, 200, "[%s]", instanceName);        
    }
    if(category) {
        curp += jm_snprintf(curp, 200, "[%s]", category);
    }
    fprintf(stdout, "%s[status=%s]", buf, fmi3_status_to_string(status));
    vfprintf (stdout, message, args);
    fprintf(stdout, "\n");
    va_end (args);
}

void fmi3_logger(jm_callbacks* cb, jm_string module, jm_log_level_enu_t log_level, jm_string message) {
	fmi3_callback_functions_t* c = (fmi3_callback_functions_t*)cb->context;
	fmi3_status_t status;
	if(!c ||!c->logger) return;

	if(log_level > jm_log_level_all) {
		assert(0);
		status = fmi3_status_error;
	}
	else if(log_level >= jm_log_level_info)
		status = fmi3_status_ok;
	else if(log_level >= jm_log_level_warning)
		status = fmi3_status_warning;
	else if(log_level >= jm_log_level_error)
		status = fmi3_status_error;
	else if(log_level >= jm_log_level_fatal)
		status = fmi3_status_fatal;
	else {
		status = fmi3_status_ok;
	}

	c->logger( c, module, status, jm_log_level_to_string(log_level), message);
}

void fmi3_import_init_logger(jm_callbacks* cb, fmi3_callback_functions_t* fmiCallbacks) {
	cb->logger = fmi3_logger;
	cb->context = fmiCallbacks;
}

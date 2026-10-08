#ifndef PD_DATA_H
#define PD_DATA_H

#include <stdint.h>

typedef struct {
    uint8_t type1;
    uint8_t type2;
} pd_species_type;

extern const char* const pd_species_names[412];
extern const pd_species_type pd_species_types[412];
extern const char* const pd_type_names[18];
extern const int pd_type_chart[18][18];
extern const char* const pd_item_names[401];

#endif // PD_DATA_H

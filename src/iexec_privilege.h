#pragma once

#include "iexec.h"

void iexec_drop_privilege(void);

void iexec_drop_privilege_permanently(void);

void iexec_prepare_pidns_privilege(void);

void iexec_privilege_snapshot(void);

void iexec_assert_exec_privilege_contract(void);

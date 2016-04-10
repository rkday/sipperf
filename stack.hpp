#pragma once

void create_sip_stacks(int how_many);
struct sip* get_sip_stack();
void close_sip_stacks(bool force);
void free_sip_stacks();

#pragma once
#include <utility>

void create_sip_stacks(int how_many);
std::pair<struct sip*, struct sipsess_sock*> get_sip_stack();
void close_sip_stacks(bool force);
void free_sip_stacks();

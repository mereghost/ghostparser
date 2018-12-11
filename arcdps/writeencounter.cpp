/* write event chain */
uint32_t writeencounter(FILE* fd, AList* al_combat, AList* al_agents, uint32_t start_type) {
	/* file byte index */
	Context* ctx = getcontext();
	uint32_t fdindex = 0;

	/* write header (16 bytes) */
	char header[32];
	asnprintf(&header[0], 32, "EVTC%s", ctx->m_version); // 12 byte date string and evtc magic
	header[12] = 0; // revision byte
	if (ctx->m_state->new_cbtevent) header[12] = 1;
	*(uint16_t*)(&header[13]) = ctx->m_game->m_area_cbt_speciesid; // 2 byte boss thats being logged
	header[15] = 0; // unused, possibly expanded to an extra byte for species id just in case
	fseek(fd, 0, SEEK_SET);
	fwrite(&header[0], 16, 1, fd);
	fdindex += 16;

	/* define agent */
	typedef struct evtc_agent {
		uint64_t addr;
		uint32_t prof;
		uint32_t is_elite;
		uint16_t toughness;
		uint16_t concentration;
		uint16_t healing;
		uint16_t hitbox_width;
		uint16_t condition;
		uint16_t hitbox_height;
		char name[64];
	} evtc_agent;

	/* count agents */
	alisti itr;
	uint32_t ag_count = al_agents->Count();
	int32_t max_toughness = 1;
	int32_t max_concentration = 1;
	int32_t max_healing = 1;
	int32_t max_condition = 1;
	evtc_agent* evag = (evtc_agent*)al_agents->IInitTail(&itr);
	while (evag) {
		if (evag->is_elite != 0xFFFFFFFF) {
			max_toughness = MAX(evag->toughness, max_toughness);
			max_concentration = MAX(evag->concentration, max_concentration);
			max_healing = MAX(evag->healing, max_healing);
			max_condition = MAX(evag->condition, max_condition);
		}
		evag = (evtc_agent*)al_agents->INext(&itr);
	}

	/* write agent count */
	fseek(fd, fdindex, SEEK_SET);
	fwrite(&ag_count, sizeof(uint32_t), 1, fd);
	fdindex += sizeof(uint32_t);

	/* write agent array */
	evag = (evtc_agent*)al_agents->IInitTail(&itr);
	while (evag) {
		if (evag->is_elite != 0xFFFFFFFF) {
			evag->toughness = ((evag->toughness * 100) / max_toughness) / 10;
			evag->concentration = ((evag->concentration * 100) / max_concentration) / 10;
			evag->healing = ((evag->healing * 100) / max_healing) / 10;
			evag->condition = ((evag->condition * 100) / max_condition) / 10;
		}
		fseek(fd, fdindex, SEEK_SET);
		fwrite(evag, sizeof(evtc_agent), 1, fd);
		fdindex += sizeof(evtc_agent);
		evag = (evtc_agent*)al_agents->INext(&itr);
	}

	/* count skills */
	uint8_t* sk_mask = (uint8_t*)acalloc(sizeof(uint8_t) * 65535);
	cbtevent_extended* cbtev = (cbtevent_extended*)al_combat->IInitTail(&itr);
	uint32_t skcount = 0;
	while (cbtev) {
		if (!sk_mask[cbtev->skillid]) {
			skcount += 1;
			sk_mask[cbtev->skillid] = 1;
		}
		cbtev = (cbtevent_extended*)al_combat->INext(&itr);
	}

	/* write skill count */
	fseek(fd, fdindex, SEEK_SET);
	fwrite(&skcount, sizeof(uint32_t), 1, fd);
	fdindex += sizeof(uint32_t);

	/* define skill */
	typedef struct evtc_skill {
		int32_t id;
		char name[64];
	} evtc_skill;

	/* write skill array */
	skcount = 0;
	while (skcount < 65535) {
		if (sk_mask[skcount]) {
			evtc_skill temp;
			memset(&temp, 0, sizeof(evtc_skill));
			temp.id = skcount;
			asnprintf(&temp.name[0], RB_NAME_LEN, "%s", ctx->m_game->GetSkill(skcount)->name);
			fseek(fd, fdindex, SEEK_SET);
			fwrite(&temp, sizeof(evtc_skill), 1, fd);
			fdindex += sizeof(evtc_skill);
		}
		skcount += 1;
	}
	acfree(sk_mask);

	/* write combat log */
	cbtev = (cbtevent_extended*)al_combat->IInitTail(&itr);
	while (cbtev) {

		/* write */
		fseek(fd, fdindex, SEEK_SET);
		fwrite(cbtev, sizeof(cbtevent), 1, fd);
		fdindex += sizeof(cbtevent);
		cbtev = (cbtevent_extended*)al_combat->INext(&itr);
	}

	/* cleanup */
	fclose(fd);
	return fdindex;
}
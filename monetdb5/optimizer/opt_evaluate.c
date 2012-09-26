/*
 * The contents of this file are subject to the MonetDB Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.monetdb.org/Legal/MonetDBLicense
 * 
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * The Original Code is the MonetDB Database System.
 * 
 * The Initial Developer of the Original Code is CWI.
 * Portions created by CWI are Copyright (C) 1997-July 2008 CWI.
 * Copyright August 2008-2012 MonetDB B.V.
 * All Rights Reserved.
*/
#include "monetdb_config.h"
#include "opt_evaluate.h"
#include "opt_aliases.h"

static int
OPTallConstant(Client cntxt, MalBlkPtr mb, InstrPtr p)
{
	int i;
	(void)cntxt;

	if (p->argc == p->retc)
		return FALSE;
	for (i = p->retc; i < p->argc; i++)
		if (isVarConstant(mb, getArg(p, i)) == FALSE)
			return FALSE;
	for (i = 0; i < p->retc; i++)
		if (isaBatType(getArgType(mb, p, i)))
			return FALSE;
	return(getModuleId(p) == calcRef ||
		   getModuleId(p) == strRef ||
		   getModuleId(p) == mmathRef ||
		   p->token == ASSIGNsymbol);
}

static int
OPTremoveUnusedBlocks(Client cntxt, MalBlkPtr mb)
{
	/* catch constant bounded blocks */
	int i, j = 0, action = 0, block = 0, skip = 0;
	InstrPtr p;

	for (i = 0; i < mb->stop; i++) {
		p = mb->stmt[i];
		if (blockStart(p)) {
			block++;
			if (p->argc == 2 && isVarConstant(mb, getArg(p, 1)) &&
					getArgType(mb, p, 1) == TYPE_bit &&
					getVarConstant(mb, getArg(p, 1)).val.btval == 0)
			{
				if (skip == 0)
					skip = block;
				action++;
			}
		}
		if (blockExit(p)) {
			if (skip)
				freeInstruction(p);
			else
				mb->stmt[j++] = p;
			if (skip == block)
				skip = 0;
			block--;
		} else if (skip)
			freeInstruction(p);
		else
			mb->stmt[j++] = p;
		if (block == 0)
			skip = 0;
	}
	mb->stop = j;
	for (; j < i; j++)
		mb->stmt[j] = NULL;
	if (action) {
		chkTypes(cntxt->fdout, cntxt->nspace, mb, TRUE);
		return mb->errors ? 0 : action;
	}
	return action;
}

static int
assignedOnce(MalBlkPtr mb, Lifespan span, int varid)
{
	int i, cnt = 0;
	for (i = getBeginLifespan(span, varid); i <= getLastUpdate(span, varid); i++)
		cnt += getArg(getInstrPtr(mb, i), 0) == varid;
	if (getInstrPtr(mb, getLastUpdate(span, varid))->barrier == EXITsymbol)
		cnt--;
	return cnt == 1;
}

int
OPTevaluateImplementation(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci)
{
	InstrPtr p;
	int i, k, limit, *alias, barrier;
	MalStkPtr env = NULL;
	int profiler;
	str msg;
	int debugstate = cntxt->itrace, actions = 0;
	Lifespan span;

	cntxt->itrace = 0;
	(void)stk;
	(void)pci;

	if (varGetProp(mb, getArg(mb->stmt[0], 0), inlineProp) != NULL)
		return 0;

	(void)cntxt;
	OPTDEBUGevaluate mnstr_printf(cntxt->fdout, "Constant expression optimizer started\n");

	span = setLifespan(mb);
	if (span == NULL)
		return 0;

	env = prepareMALstack(mb, 2 * mb->vsize);
	env->keepAlive = TRUE;
	alias = (int*)GDKzalloc(mb->vsize * sizeof(int) * 2); /* we introduce more */
	if (alias == NULL)
		return 0;

	limit = mb->stop;
	for (i = 1; i < limit; i++) {
		p = getInstrPtr(mb, i);
		for (k = p->retc; k < p->argc; k++)
			if (alias[getArg(p, k)])
				getArg(p, k) = alias[getArg(p, k)];
		OPTDEBUGevaluate printInstruction(cntxt->fdout, mb, 0, p, LIST_MAL_ALL);
		/* be aware that you only assign once to a variable */
		if (p->retc == 1 && OPTallConstant(cntxt, mb, p) && !isUnsafeFunction(p)) {
			if (assignedOnce(mb, span, getArg(p, 0)) == FALSE)
				continue;
			actions++;
			barrier = p->barrier;
			p->barrier = 0;
			profiler = malProfileMode;	/* we don't trace it */
			malProfileMode = 0;
			msg = reenterMAL(cntxt, mb, i, i + 1, env);
			malProfileMode= profiler;
			p->barrier = barrier;
			OPTDEBUGevaluate {
				mnstr_printf(cntxt->fdout, "#retc var %s\n", getVarName(mb, getArg(p, 0)));
				mnstr_printf(cntxt->fdout, "#result:%s\n", msg == MAL_SUCCEED ? "ok" : msg);
			}
			if (msg == MAL_SUCCEED) {
				int nvar;
				ValRecord cst;

				cst.vtype = 0;
				VALcopy(&cst, &env->stk[getArg(p, 0)]);
				/* You may not overwrite constants.  They may be used by
				 * other instructions */
				nvar = getArg(p, 1) = defConstant(mb, getArgType(mb, p, 0), &cst);
				if (nvar >= env->stktop) {
					VALcopy(&env->stk[getArg(p, 1)], &getVarConstant(mb, getArg(p, 1)));
					env->stktop = getArg(p, 1) + 1;
				}
				alias[getArg(p, 0)] = getArg(p, 1);
				p->argc = 2;
				p->token = ASSIGNsymbol;
				clrFunction(p);
				/* freeze the type */
				setVarFixed(mb,getArg(p,1));
				setVarUDFtype(mb,getArg(p,1));
				OPTDEBUGevaluate {
					mnstr_printf(cntxt->fdout, "Evaluated new constant=%d -> %d:%s\n",
						getArg(p, 0), getArg(p, 1), getTypeName(getArgType(mb, p, 1)));
				}
			} else {
				/* if there is an error, we should postpone message handling,
					as the actual error (eg. division by zero ) may not happen) */
				OPTDEBUGevaluate mnstr_printf(cntxt->fdout, "Evaluated %s\n", msg);
				GDKfree(msg);
				mb->errors = 0;
			}
		}
	}
	actions += OPTremoveUnusedBlocks(cntxt, mb);
	DEBUGoptimizers
		mnstr_printf(cntxt->fdout, "#opt_evaluate: %d constant expressions\n", actions);
	GDKfree(span);
	GDKfree(alias);
	freeStack(env);
	cntxt->itrace = debugstate;
	return actions;
}

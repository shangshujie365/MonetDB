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
 *                * Copyright August 2008-2014 MonetDB B.V.
 * All Rights Reserved.
 */

/*
 * (c)2014 author Martin Kersten
 * Range encoding framework for a single chunk
 */

#include "monetdb_config.h"
#include "mosaic.h"
#include "mosaic_linear.h"

#define linear_base(BLK) ((void*)(((char*) BLK)+ MosaicBlkSize))

static void*
linear_step(MOStask task, MosaicBlk blk){
	switch(ATOMstorage(task->type)){
	case TYPE_bte : return (void*) ( ((char*)blk)+ MosaicBlkSize+ sizeof(bte));
	case TYPE_bit : return (void*) ( ((char*)blk)+ MosaicBlkSize+ sizeof(bit));
	case TYPE_sht : return (void*) ( ((char*)blk)+ MosaicBlkSize+ sizeof(sht));
	case TYPE_int : return (void*) ( ((char*)blk)+ MosaicBlkSize+ sizeof(int));
	case TYPE_lng : return (void*) ( ((char*)blk)+ MosaicBlkSize+ sizeof(lng));
#ifdef HAVE_HGE
	case TYPE_hge : return (void*) ( ((char*)blk)+ MosaicBlkSize+ sizeof(hge));
#endif
	case TYPE_wrd : return (void*) ( ((char*)blk)+ MosaicBlkSize+ sizeof(wrd));
	case TYPE_oid : return (void*) ( ((char*)blk)+ MosaicBlkSize+ sizeof(oid));
	case TYPE_flt : return (void*) ( ((char*)blk)+ MosaicBlkSize+ sizeof(flt));
	case TYPE_dbl : return (void*) ( ((char*)blk)+ MosaicBlkSize+ sizeof(dbl));
	}
	return 0;
}

/* Beware, the dump routines use the compressed part of the task */
void
MOSdump_linear(Client cntxt, MOStask task)
{
	MosaicBlk blk= task->blk;

	mnstr_printf(cntxt->fdout,"#linear "BUNFMT" ", MOScnt(blk));
	switch(ATOMstorage(task->type)){
	case TYPE_bte:
		mnstr_printf(cntxt->fdout,"bte %d %d", *(int*) linear_base(blk), *(int*) linear_step(task, blk)); break;
	case TYPE_sht:
		mnstr_printf(cntxt->fdout,"sht %d %d", *(int*) linear_base(blk), *(int*) linear_step(task,blk)); break;
	case TYPE_int:
		mnstr_printf(cntxt->fdout,"int %d %d", *(int*) linear_base(blk), *(int*) linear_step(task,blk)); break;
	case  TYPE_lng:
		mnstr_printf(cntxt->fdout,"int "LLFMT" " LLFMT, *(lng*) linear_base(blk), *(lng*) linear_step(task,blk)); break;
#ifdef HAVE_HGE
	case  TYPE_hge:
		mnstr_printf(cntxt->fdout,"int %.40g %.40g ", (dbl) *(hge*) linear_base(blk), (dbl) *(hge*) linear_step(task,blk)); break;
#endif
	case  TYPE_wrd:
		mnstr_printf(cntxt->fdout,"int "SZFMT" " SZFMT, *(wrd*) linear_base(blk), *(wrd*) linear_step(task,blk)); break;
	case TYPE_flt:
		mnstr_printf(cntxt->fdout,"flt  %f %f", *(flt*) linear_base(blk), *(flt*) linear_step(task,blk)); break;
	case TYPE_dbl:
		mnstr_printf(cntxt->fdout,"flt  %f %f", *(dbl*) linear_base(blk), *(dbl*) linear_step(task,blk)); break;
	default:
		if( task->type == TYPE_daytime)
			mnstr_printf(cntxt->fdout,"daytime %d %d", *(int*) linear_base(blk), *(int*) linear_step(task,blk)); 
		if( task->type == TYPE_date)
			mnstr_printf(cntxt->fdout,"date %d %d", *(int*) linear_base(blk), *(int*) linear_step(task,blk)); 
		if( task->type == TYPE_timestamp)
			mnstr_printf(cntxt->fdout,"int "LLFMT" " LLFMT, *(lng*) linear_base(blk), *(lng*) linear_step(task,blk)); 
	}
	mnstr_printf(cntxt->fdout,"\n");
}

void
MOSadvance_linear(Client cntxt, MOStask task)
{
	(void) cntxt;
	task->start += MOScnt(task->blk);
	switch(task->type){
	case TYPE_bte: task->blk = (MosaicBlk)( ((char*)task->blk) + MosaicBlkSize + wordaligned(2 * sizeof(bte),bte)); break;
	case TYPE_bit: task->blk = (MosaicBlk)( ((char*)task->blk) + MosaicBlkSize + wordaligned(2 * sizeof(bit),bit)); break;
	case TYPE_sht: task->blk = (MosaicBlk)( ((char*)task->blk) + MosaicBlkSize + wordaligned(2 * sizeof(sht),sht)); break;
	case TYPE_int: task->blk = (MosaicBlk)( ((char*)task->blk) + MosaicBlkSize + wordaligned(2 * sizeof(int),int)); break;
	case TYPE_oid: task->blk = (MosaicBlk)( ((char*)task->blk) + MosaicBlkSize + wordaligned(2 * sizeof(oid),oid)); break;
	case TYPE_lng: task->blk = (MosaicBlk)( ((char*)task->blk) + MosaicBlkSize + wordaligned(2 * sizeof(lng),lng)); break;
#ifdef HAVE_HGE
	case TYPE_hge: task->blk = (MosaicBlk)( ((char*)task->blk) + MosaicBlkSize + wordaligned(2 * sizeof(hge),hge)); break;
#endif
	case TYPE_wrd: task->blk = (MosaicBlk)( ((char*)task->blk) + MosaicBlkSize + wordaligned(2 * sizeof(wrd),wrd)); break;
	case TYPE_flt: task->blk = (MosaicBlk)( ((char*)task->blk) + MosaicBlkSize + wordaligned(2 * sizeof(flt),flt)); break;
	case TYPE_dbl: task->blk = (MosaicBlk)( ((char*)task->blk) + MosaicBlkSize + wordaligned(2 * sizeof(dbl),dbl)); break;
	default:
		if( task->type == TYPE_daytime)
			task->blk = (MosaicBlk)( ((char*)task->blk) + MosaicBlkSize + wordaligned(2 * sizeof(daytime), daytime) ); 
		if( task->type == TYPE_date)
			task->blk = (MosaicBlk)( ((char*)task->blk) + MosaicBlkSize + wordaligned(2 * sizeof(date),date) ); 
		if( task->type == TYPE_timestamp)
			task->blk = (MosaicBlk)( ((char*)task->blk) + MosaicBlkSize + wordaligned(2 * sizeof(timestamp),timestamp) ); 
	}
}

void
MOSskip_linear(Client cntxt, MOStask task)
{
	MOSadvance_linear(cntxt, task);
	if ( MOStag(task->blk) == MOSAIC_EOL)
		task->blk = 0; // ENDOFLIST
}

#define Estimate(TYPE)\
{	TYPE val = *(TYPE*) task->src;\
	TYPE step = *(TYPE*) (task->src + sizeof(TYPE)) - val;\
	for(i =1; i < task->elm; i++)\
	if ( ((TYPE*)task->src)[i] != (TYPE)(val + (int)i * step))\
		break;\
	factor =  ( (flt)i * sizeof(TYPE))/(MosaicBlkSize + 2 * sizeof(TYPE));\
}

// calculate the expected reduction using LINEAR in terms of elements compressed
flt
MOSestimate_linear(Client cntxt, MOStask task)
{	BUN i = -1;
	flt factor = 1.0;
	(void) cntxt;

	switch(ATOMstorage(task->type)){
	case TYPE_bte: Estimate(bte); break;
	case TYPE_bit: Estimate(bit); break;
	case TYPE_sht: Estimate(sht); break;
	case TYPE_oid: Estimate(oid); break;
	case TYPE_lng: Estimate(lng); break;
#ifdef HAVE_HGE
	case TYPE_hge: Estimate(hge); break;
#endif
	case TYPE_wrd: Estimate(wrd); break;
	case TYPE_flt: Estimate(flt); break;
	case TYPE_dbl: Estimate(dbl); break;
	case TYPE_int:
		{	int val = *(int*)task->src;
			int step = *(int*) (task->src + sizeof(int)) - val;
			for(i =1; i<task->elm; i++)
			if ( ((int*)task->src)[i] != (int)(val + (int)i * step))
				break;
			factor =  ( (flt)i * sizeof(int))/(MosaicBlkSize + 2 * sizeof(int));
		}
	}
#ifdef _DEBUG_MOSAIC_
	mnstr_printf(cntxt->fdout,"#estimate linear %d elm %4.2f factor\n",(int)i,factor);
#endif
	return factor;
}

// insert a series of values into the compressor block using linear.
#define LINEARcompress(TYPE)\
{	TYPE val = *(TYPE*) task->src;\
	TYPE step = *(TYPE*) (task->src + sizeof(TYPE)) - val;\
	for(i =1; i<task->elm; i++)\
	if ( ((TYPE*)task->src)[i] != (TYPE)(val + (int)i * step))\
		break;\
	MOSinc(blk,i);\
	*(TYPE*) linear_base(blk) = val;\
	*(TYPE*) linear_step(task,blk) = step;\
	task->dst = ((char*) blk)+ MosaicBlkSize +  2 * sizeof(TYPE);\
	task->src += i * sizeof(TYPE);\
}

void
MOScompress_linear(Client cntxt, MOStask task)
{
	BUN i ;
	MosaicBlk blk = task->blk;

	(void) cntxt;
	*blk = MOSlinear;

	switch(ATOMstorage(task->type)){
	case TYPE_bte: LINEARcompress(bte); break;
	case TYPE_bit: LINEARcompress(bit); break;
	case TYPE_sht: LINEARcompress(sht); break;
	case TYPE_oid: LINEARcompress(oid); break;
	case TYPE_lng: LINEARcompress(lng); break;
#ifdef HAVE_HGE
	case TYPE_hge: LINEARcompress(hge); break;
#endif
	case TYPE_wrd: LINEARcompress(wrd); break;
	case TYPE_flt: LINEARcompress(flt); break;
	case TYPE_dbl: LINEARcompress(dbl); break;
	case TYPE_int:
		{	int val = *(int*) task->src;\
			int step = *(int*) (task->src + sizeof(int)) - val;\
			for(i =1; i<task->elm; i++)\
			if ( ((int*)task->src)[i] != (int)(val + (int)i * step))\
				break;\
			MOSinc(blk,i);\
			*(int*) linear_base(blk) = val;\
			*(int*) linear_step(task,blk) = step;\
			task->dst = ((char*) blk)+ MosaicBlkSize +  2 * sizeof(TYPE);
			task->src += i * sizeof(int);\
		}
		break;
	}
#ifdef _DEBUG_MOSAIC_
	MOSdump_linear(cntxt, task);
#endif
}

// the inverse operator, extend the src
#define LINEARdecompress(TYPE)\
{	TYPE val = *(TYPE*) linear_base(blk);\
	TYPE step = *(TYPE*) linear_step(task,blk);\
	BUN lim = MOScnt(blk);\
	for(i = 0; i < lim; i++)\
		((TYPE*)task->src)[i] = val + (int) i * step;\
	task->src += i * sizeof(TYPE);\
}

void
MOSdecompress_linear(Client cntxt, MOStask task)
{
	MosaicBlk blk =  task->blk;
	BUN i;
	(void) cntxt;

	switch(ATOMstorage(task->type)){
	case TYPE_bte: LINEARdecompress(bte); break ;
	case TYPE_bit: LINEARdecompress(bit); break ;
	case TYPE_sht: LINEARdecompress(sht); break;
	case TYPE_oid: LINEARdecompress(oid); break;
	case TYPE_lng: LINEARdecompress(lng); break;
#ifdef HAVE_HGE
	case TYPE_hge: LINEARdecompress(hge); break;
#endif
	case TYPE_wrd: LINEARdecompress(wrd); break;
	case TYPE_flt: LINEARdecompress(flt); break;
	case TYPE_dbl: LINEARdecompress(dbl); break;
	case TYPE_int:
		{	int val = *(int*) linear_base(blk) ;
			int step = *(int*) linear_step(task,blk);
			BUN lim= MOScnt(blk);
			for(i = 0; i < lim; i++)
				((int*)task->src)[i] = val + i * step;
			task->src += i * sizeof(int);
		}
	}
}

// perform relational algebra operators over non-compressed chunks
// They are bound by an oid linear and possibly a candidate list

#define  subselect_linear(TYPE) \
{	TYPE val = *(TYPE*) linear_base(blk) ;\
	TYPE step = *(TYPE*) linear_step(task,blk);\
	if( !*anti){\
		if( *(TYPE*) low == TYPE##_nil && *(TYPE*) hgh == TYPE##_nil){\
			for( ; first < last; first++){\
				MOSskipit();\
				*o++ = (oid) first;\
			}\
		} else\
		if( *(TYPE*) low == TYPE##_nil ){\
			for( ; first < last; first++, val+=step){\
				MOSskipit();\
				cmp  =  ((*hi && val <= * (TYPE*)hgh ) || (!*hi && val < *(TYPE*)hgh ));\
				if (cmp )\
					*o++ = (oid) first;\
			}\
		} else\
		if( *(TYPE*) hgh == TYPE##_nil ){\
			for( ; first < last; first++, val+= step){\
				MOSskipit();\
				cmp  =  ((*li && val >= * (TYPE*)low ) || (!*li && val > *(TYPE*)low ));\
				if (cmp )\
					*o++ = (oid) first;\
			}\
		} else{\
			for( ; first < last; first++,val+=step){\
				MOSskipit();\
				cmp  =  ((*hi && val <= * (TYPE*)hgh ) || (!*hi && val < *(TYPE*)hgh )) &&\
						((*li && val >= * (TYPE*)low ) || (!*li && val > *(TYPE*)low ));\
				if (cmp )\
					*o++ = (oid) first;\
			}\
		}\
	} else {\
		if( *(TYPE*) low == TYPE##_nil && *(TYPE*) hgh == TYPE##_nil){\
			/* nothing is matching */\
		} else\
		if( *(TYPE*) low == TYPE##_nil ){\
			for( ; first < last; first++, val+=step){\
				MOSskipit();\
				cmp  =  ((*hi && val <= * (TYPE*)hgh ) || (!*hi && val < *(TYPE*)hgh ));\
				if ( !cmp )\
					*o++ = (oid) first;\
			}\
		} else\
		if( *(TYPE*) hgh == TYPE##_nil ){\
			for( ; first < last; first++, val+=step){\
				MOSskipit();\
				cmp  =  ((*li && val >= * (TYPE*)low ) || (!*li && val > *(TYPE*)low ));\
				if ( !cmp )\
					*o++ = (oid) first;\
			}\
		} else{\
			for( ; first < last; first++, val+=step){\
				MOSskipit();\
				cmp  =  ((*hi && val <= * (TYPE*)hgh ) || (!*hi && val < *(TYPE*)hgh )) &&\
						((*li && val >= * (TYPE*)low ) || (!*li && val > *(TYPE*)low ));\
				if (!cmp)\
					*o++ = (oid) first;\
			}\
		}\
	}\
}

str
MOSsubselect_linear(Client cntxt,  MOStask task, void *low, void *hgh, bit *li, bit *hi, bit *anti){
	oid *o;
	BUN first,last;
	int cmp;
	MosaicBlk blk =  task->blk;
	(void) cntxt;

	// set the oid range covered and advance scan range
	first = task->start;
	last = first + MOScnt(blk);

	if (task->cl && *task->cl > last){
		MOSskip_linear(cntxt,task);
		return MAL_SUCCEED;
	}
	o = task->lb;

	switch(ATOMstorage(task->type)){
	case TYPE_bit: subselect_linear(bit); break;
	case TYPE_bte: subselect_linear(bte); break;
	case TYPE_sht: subselect_linear(sht); break;
	case TYPE_oid: subselect_linear(oid); break;
	case TYPE_lng: subselect_linear(lng); break;
#ifdef HAVE_HGE
	case TYPE_hge: subselect_linear(hge); break;
#endif
	case TYPE_wrd: subselect_linear(wrd); break;
	case TYPE_flt: subselect_linear(flt); break;
	case TYPE_dbl: subselect_linear(dbl); break;
	case TYPE_int:
	// Expanded MOSselect_linear for debugging
	{	int val = *(int*) linear_base(blk) ;
		int step = *(int*) linear_step(task,blk);

		if( !*anti){
			if( *(int*) low == int_nil && *(int*) hgh == int_nil){
				for( ; first < last; first++){
					MOSskipit();
					*o++ = (oid) first;
				}
			} else
			if( *(int*) low == int_nil ){
				for( ; first < last; first++, val+=step){
					MOSskipit();
					cmp  =  ((*hi && val <= * (int*)hgh ) || (!*hi && val < *(int*)hgh ));
					if (cmp )
						*o++ = (oid) first;
				}
			} else
			if( *(int*) hgh == int_nil ){
				for( ; first < last; first++, val+=step){
					MOSskipit();
					cmp  =  ((*li && val >= * (int*)low ) || (!*li && val > *(int*)low ));
					if (cmp )
						*o++ = (oid) first;
				}
			} else{
				for( ; first < last; first++, val+=step){
					MOSskipit();
					cmp  =  ((*hi && val <= * (int*)hgh ) || (!*hi && val < *(int*)hgh )) &&
							((*li && val >= * (int*)low ) || (!*li && val > *(int*)low ));
					if (cmp )
						*o++ = (oid) first;
				}
			}
		} else {
			if( *(int*) low == int_nil && *(int*) hgh == int_nil){
				/* nothing is matching */
			} else
			if( *(int*) low == int_nil ){
				for( ; first < last; first++, val+=step){
					MOSskipit();
					cmp  =  ((*hi && val <= * (int*)hgh ) || (!*hi && val < *(int*)hgh ));
					if ( !cmp )
						*o++ = (oid) first;
				}
			} else
			if( *(int*) hgh == int_nil ){
				for( ; first < last; first++, val+=step){
					MOSskipit();
					cmp  =  ((*li && val >= * (int*)low ) || (!*li && val > *(int*)low ));
					if ( !cmp )
						*o++ = (oid) first;
				}
			} else{
				for( ; first < last; first++, val+=step){
					MOSskipit();
					cmp  =  ((*hi && val <= * (int*)hgh ) || (!*hi && val < *(int*)hgh )) &&
							((*li && val >= * (int*)low ) || (!*li && val > *(int*)low ));
					if (!cmp)
						*o++ = (oid) first;
				}
			}
		}
	}
	break;
	default:
		if( task->type == TYPE_daytime)
			subselect_linear(daytime); 
		if( task->type == TYPE_date)
			subselect_linear(date); 
		if( task->type == TYPE_timestamp)
			subselect_linear(lng); 
	}
	MOSskip_linear(cntxt,task);
	task->lb = o;
	return MAL_SUCCEED;
}

#define thetasubselect_linear(TYPE)\
{ 	TYPE low,hgh;\
	TYPE v = *(TYPE*) linear_base(blk) ;\
	TYPE step = *(TYPE*) linear_step(task,blk);\
	low= hgh = TYPE##_nil;\
	if ( strcmp(oper,"<") == 0){\
		hgh= *(TYPE*) val;\
		hgh = PREVVALUE##TYPE(hgh);\
	} else\
	if ( strcmp(oper,"<=") == 0){\
		hgh= *(TYPE*) val;\
	} else\
	if ( strcmp(oper,">") == 0){\
		low = *(TYPE*) val;\
		low = NEXTVALUE##TYPE(low);\
	} else\
	if ( strcmp(oper,">=") == 0){\
		low = *(TYPE*) val;\
	} else\
	if ( strcmp(oper,"!=") == 0){\
		low = hgh = *(TYPE*) val;\
		anti++;\
	} else\
	if ( strcmp(oper,"==") == 0){\
		hgh= low= *(TYPE*) val;\
	} \
	if ( !anti) {\
		for( ; first < last; first++, v+=step){\
			MOSskipit();\
			if( (low == TYPE##_nil || v >= low) && (v <= hgh || hgh == TYPE##_nil) )\
				*o++ = (oid) first;\
		}\
	} else\
	if( anti)\
		for( ; first < last; first++, v+=step){\
			MOSskipit();\
			if( !( (low == TYPE##_nil || v >= low) && (v <= hgh || hgh == TYPE##_nil) ))\
			*o++ = (oid) first;\
		}\
}

str
MOSthetasubselect_linear(Client cntxt,  MOStask task,void *val, str oper)
{
	oid *o;
	int anti=0;
	BUN first,last;
	MosaicBlk blk= task->blk;
	(void) cntxt;
	
	// set the oid range covered and advance scan range
	first = task->start;
	last = first + MOScnt(task->blk);

	if (task->cl && *task->cl > last){
		MOSskip_linear(cntxt,task);
		return MAL_SUCCEED;
	}
	o = task->lb;

	switch(task->type){
	case TYPE_bit: thetasubselect_linear(bit); break;
	case TYPE_bte: thetasubselect_linear(bte); break;
	case TYPE_sht: thetasubselect_linear(sht); break;
	case TYPE_oid: thetasubselect_linear(oid); break;
	case TYPE_lng: thetasubselect_linear(lng); break;
#ifdef HAVE_HGE
	case TYPE_hge: thetasubselect_linear(hge); break;
#endif
	case TYPE_wrd: thetasubselect_linear(wrd); break;
	case TYPE_flt: thetasubselect_linear(flt); break;
	case TYPE_dbl: thetasubselect_linear(dbl); break;
	case TYPE_int:
		{ 	int low,hgh;
			int v = *(int*) linear_base(blk) ;
			int step = *(int*) linear_step(task,blk);
			low= hgh = int_nil;
			if ( strcmp(oper,"<") == 0){
				hgh= *(int*) val;
				hgh = PREVVALUEint(hgh);
			} else
			if ( strcmp(oper,"<=") == 0){
				hgh= *(int*) val;
			} else
			if ( strcmp(oper,">") == 0){
				low = *(int*) val;
				low = NEXTVALUEint(low);
			} else
			if ( strcmp(oper,">=") == 0){
				low = *(int*) val;
			} else
			if ( strcmp(oper,"!=") == 0){
				low = hgh = *(int*) val;
				anti++;
			} else
			if ( strcmp(oper,"==") == 0){
				hgh= low= *(int*) val;
			} 
			if ( !anti) {
				for( ; first < last; first++, v+=step){
					MOSskipit();
					if( (low == int_nil || v >= low) && (v <= hgh || hgh == int_nil) )
							*o++ = (oid) first;
				}
			} else
			if( anti)
				for( ; first < last; first++,v+=step){
					MOSskipit();
					if( !((low == int_nil || v >= low) && (v <= hgh || hgh == int_nil) ))
						*o++ = (oid) first;
				}
		}
		break;
	default:
		if( task->type == TYPE_date)
			thetasubselect_linear(date); 
		if( task->type == TYPE_daytime)
			thetasubselect_linear(daytime); 
		if( task->type == TYPE_timestamp)
			thetasubselect_linear(lng); 
	}
	MOSskip_linear(cntxt,task);
	task->lb =o;
	return MAL_SUCCEED;
}

#define leftfetchjoin_linear(TYPE)\
{TYPE *v;\
	TYPE val = *(TYPE*) linear_base(blk) ;\
	TYPE step = *(TYPE*) linear_step(task,blk);\
	v= (TYPE*) task->src;\
	for(; first < last; first++, val+=step){\
		MOSskipit();\
		*v++ = val;\
		task->n--;\
	}\
	task->src = (char*) v;\
}

str
MOSleftfetchjoin_linear(Client cntxt,  MOStask task)
{
	BUN first,last;
	MosaicBlk blk = task->blk;

	(void) cntxt;
	// set the oid range covered and advance scan range
	first = task->start;
	last = first + MOScnt(task->blk);

	switch(task->type){
		case TYPE_bit: leftfetchjoin_linear(bit); break;
		case TYPE_bte: leftfetchjoin_linear(bte); break;
		case TYPE_sht: leftfetchjoin_linear(sht); break;
		case TYPE_oid: leftfetchjoin_linear(oid); break;
		case TYPE_lng: leftfetchjoin_linear(lng); break;
#ifdef HAVE_HGE
		case TYPE_hge: leftfetchjoin_linear(hge); break;
#endif
		case TYPE_wrd: leftfetchjoin_linear(wrd); break;
		case TYPE_flt: leftfetchjoin_linear(flt); break;
		case TYPE_dbl: leftfetchjoin_linear(dbl); break;
		case TYPE_int:
		{	int *v;
			int val = *(int*) linear_base(blk) ;
			int step = *(int*) linear_step(task,blk);
			v= (int*) task->src;
			for(; first < last; first++,val+=step){
				MOSskipit();
				*v++ = val;
				task->n--;
			}
			task->src = (char*) v;
		}
		break;
		default:
			if( task->type == TYPE_daytime)
				leftfetchjoin_linear(daytime); 
			if( task->type == TYPE_date)
				leftfetchjoin_linear(date); 
			if( task->type == TYPE_timestamp)
			{	lng *v;
				lng val = *(lng*) linear_base(blk) ;
				lng step = *(lng*) linear_step(task,blk);
				v= (lng*) task->src;
				for(; first < last; first++, val+=step){
					MOSskipit();
					*v++ = val;
					task->n--;
				}
				task->src = (char*) v;
			}
	}
	MOSskip_linear(cntxt,task);
	return MAL_SUCCEED;
}

#define join_linear(TYPE)\
{	TYPE *w;\
	TYPE val = *(TYPE*) linear_base(blk) ;\
	TYPE step = *(TYPE*) linear_step(task,blk);\
	w = (TYPE*) task->src;\
	for(n = task->elm, o = 0; n -- > 0; w++,o++)\
		for(oo= (oid) first, val = *(int*) linear_base(blk); oo < (oid) last; val+=step, oo++)\
		if ( *w == val){\
			BUNappend(task->lbat, &oo, FALSE);\
			BUNappend(task->rbat, &o, FALSE);\
		}\
}

str
MOSjoin_linear(Client cntxt,  MOStask task)
{
	MosaicBlk blk = task->blk;
	BUN n,first,last;
	oid o, oo;

	(void) cntxt;
	// set the oid range covered and advance scan range
	first = task->start;
	last = first + MOScnt(task->blk);

	switch(ATOMstorage(task->type)){
		case TYPE_bit: join_linear(bit); break;
		case TYPE_bte: join_linear(bte); break;
		case TYPE_sht: join_linear(sht); break;
		case TYPE_oid: join_linear(oid); break;
		case TYPE_lng: join_linear(lng); break;
#ifdef HAVE_HGE
		case TYPE_hge: join_linear(hge); break;
#endif
		case TYPE_wrd: join_linear(wrd); break;
		case TYPE_flt: join_linear(flt); break;
		case TYPE_dbl: join_linear(dbl); break;
		case TYPE_int:
			{	int *w;
				int val = *(int*) linear_base(blk) ;
				int step = *(int*) linear_step(task,blk);
				w = (int*) task->src;
				for(n = task->elm, o = 0; n -- > 0; w++,o++, val+=step)
					for(oo= (oid) first, val = *(int*) linear_base(blk); oo < (oid) last; val+=step, oo++)
					if ( *w == val){
						BUNappend(task->lbat, &oo, FALSE);
						BUNappend(task->rbat, &o, FALSE);
					}
			}
	}
	MOSskip_linear(cntxt,task);
	return MAL_SUCCEED;
}

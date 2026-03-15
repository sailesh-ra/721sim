///////////////////////////////////////////////////////////////////////
////  Copyright 2015 Samsung Austin Semiconductor, LLC.                //
/////////////////////////////////////////////////////////////////////////
#include "tage-sc-l.h"

void tagescl_t::reinit() {
   m[1] = MINHIST;
   m[NHIST / 2] = MAXHIST;
   for (int i = 2; i <= NHIST / 2; i++) {
      m[i] =
         (int) (((double) MINHIST *
                 pow((double) (MAXHIST) / (double) MINHIST,
                     (double) (i - 1) / (double) (((NHIST / 2) - 1)))) +
                0.5);
      //      fprintf(stderr, "(%d %d)", m[i],i);
   }
   for (int i = 1; i <= NHIST; i++) {
      NOSKIP[i] = ((i - 1) & 1) || ((i >= BORNINFASSOC) & (i < BORNSUPASSOC));
   }

   NOSKIP[4] = 0;
   NOSKIP[NHIST - 2] = 0;
   NOSKIP[8] = 0;
   NOSKIP[NHIST - 6] = 0;
   // just eliminate some extra tables (very very marginal)

   for (int i = NHIST; i > 1; i--) {
      m[i] = m[(i + 1) / 2];
   }
   for (int i = 1; i <= NHIST; i++) {
      TB[i] = TBITS + 4 * (i >= BORN);
      logg[i] = LOGG;
   }

#ifdef LOOPPREDICTOR
   ltable = new lentry[1 << (LOGL)];
#endif

   gtable[1] = new gentry[NBANKLOW * (1 << LOGG)];
   SizeTable[1] = NBANKLOW * (1 << LOGG);

   gtable[BORN] = new gentry[NBANKHIGH * (1 << LOGG)];
   SizeTable[BORN] = NBANKHIGH * (1 << LOGG);

   for (int i = BORN + 1; i <= NHIST; i++)
      gtable[i] = gtable[BORN];
   for (int i = 2; i <= BORN - 1; i++)
      gtable[i] = gtable[1];
   btable = new bentry[1 << LOGB];

   for (int i = 1; i <= NHIST; i++) {
      ch_i[i].init(m[i], (logg[i]));
      ch_t[0][i].init(ch_i[i].OLENGTH, TB[i]);
      ch_t[1][i].init(ch_i[i].OLENGTH, TB[i] - 1);
   }
#ifdef LOOPPREDICTOR
   LVALID = false;
   WITHLOOP = -1;
#endif
   Seed = 0;

   TICK = 0;
   phist = 0;
   Seed = 0;

   for (int i = 0; i < HISTBUFFERLENGTH; i++)
      ghist[0] = 0;
   ptghist = 0;
   updatethreshold = 35 << 3;

   for (int i = 0; i < (1 << LOGSIZEUP); i++)
      Pupdatethreshold[i] = 0;
   for (int i = 0; i < GNB; i++)
      GGEHL[i] = &GGEHLA[i][0];
   for (int i = 0; i < LNB; i++)
      LGEHL[i] = &LGEHLA[i][0];

   for (int i = 0; i < GNB; i++)
      for (int j = 0; j < ((1 << LOGGNB) - 1); j++) {
         if (!(j & 1)) {
            GGEHL[i][j] = -1;
         }
      }
   for (int i = 0; i < LNB; i++)
      for (int j = 0; j < ((1 << LOGLNB) - 1); j++) {
         if (!(j & 1)) {
            LGEHL[i][j] = -1;
         }
      }

   for (int i = 0; i < SNB; i++)
      SGEHL[i] = &SGEHLA[i][0];
   for (int i = 0; i < TNB; i++)
      TGEHL[i] = &TGEHLA[i][0];
   for (int i = 0; i < PNB; i++)
      PGEHL[i] = &PGEHLA[i][0];
#ifdef IMLI
#ifdef IMLIOH
   for (int i = 0; i < FNB; i++)
      FGEHL[i] = &FGEHLA[i][0];

   for (int i = 0; i < FNB; i++)
      for (int j = 0; j < ((1 << LOGFNB) - 1); j++) {
         if (!(j & 1)) {
            FGEHL[i][j] = -1;
         }
      }
#endif
   for (int i = 0; i < INB; i++)
      IGEHL[i] = &IGEHLA[i][0];
   for (int i = 0; i < INB; i++)
      for (int j = 0; j < ((1 << LOGINB) - 1); j++) {
         if (!(j & 1)) {
            IGEHL[i][j] = -1;
         }
      }
   for (int i = 0; i < IMNB; i++)
      IMGEHL[i] = &IMGEHLA[i][0];
   for (int i = 0; i < IMNB; i++)
      for (int j = 0; j < ((1 << LOGIMNB) - 1); j++) {
         if (!(j & 1)) {
            IMGEHL[i][j] = -1;
         }
      }

#endif
   for (int i = 0; i < SNB; i++)
      for (int j = 0; j < ((1 << LOGSNB) - 1); j++) {
         if (!(j & 1)) {
            SGEHL[i][j] = -1;
         }
      }
   for (int i = 0; i < TNB; i++)
      for (int j = 0; j < ((1 << LOGTNB) - 1); j++) {
         if (!(j & 1)) {
            TGEHL[i][j] = -1;
         }
      }
   for (int i = 0; i < PNB; i++)
      for (int j = 0; j < ((1 << LOGPNB) - 1); j++) {
         if (!(j & 1)) {
            PGEHL[i][j] = -1;
         }
      }

   for (int i = 0; i < (1 << LOGB); i++) {
      btable[i].pred = 0;
      btable[i].hyst = 1;
   }

   for (int j = 0; j < (1 << LOGBIAS); j++) {
      switch (j & 3) {
      case 0:
         BiasSK[j] = -8;
         break;
      case 1:
         BiasSK[j] = 7;
         break;
      case 2:
         BiasSK[j] = -32;

         break;
      case 3:
         BiasSK[j] = 31;
         break;
      }
   }
   for (int j = 0; j < (1 << LOGBIAS); j++) {
      switch (j & 3) {
      case 0:
         Bias[j] = -32;

         break;
      case 1:
         Bias[j] = 31;
         break;
      case 2:
         Bias[j] = -1;
         break;
      case 3:
         Bias[j] = 0;
         break;
      }
   }
   for (int j = 0; j < (1 << LOGBIAS); j++) {
      switch (j & 3) {
      case 0:
         BiasBank[j] = -32;

         break;
      case 1:
         BiasBank[j] = 31;
         break;
      case 2:
         BiasBank[j] = -1;
         break;
      case 3:
         BiasBank[j] = 0;
         break;
      }
   }
   for (int i = 0; i < SIZEUSEALT; i++) {
      use_alt_on_na[i] = 0;
   }
   for (int i = 0; i < (1 << LOGSIZEUPS); i++) {
      WG[i] = 7;
      WL[i] = 7;
      WS[i] = 7;
      WT[i] = 7;
      WP[i] = 7;
      WI[i] = 7;
      WB[i] = 4;
   }
   TICK = 0;
   for (int i = 0; i < NLOCAL; i++) {
      L_shist[i] = 0;
   }
   for (int i = 0; i < NSECLOCAL; i++) {
      S_slhist[i] = 0;
   }
   GHIST = 0;
   ptghist = 0;
   phist = 0;
}

// index function for the bimodal table

int tagescl_t::bindex(UINT64 PC) {
   return ((PC ^ (PC >> LOGB)) & ((1 << (LOGB)) - 1));
}

// the index functions for the tagged tables uses path history as in the OGEHL predictor
//F serves to mix path history: not very important impact

int tagescl_t::F(long long A, int size, int bank) {
   int A1, A2;
   A = A & ((1 << size) - 1);
   A1 = (A & ((1 << logg[bank]) - 1));
   A2 = (A >> logg[bank]);

   if (bank < logg[bank])
      A2 =
         ((A2 << bank) & ((1 << logg[bank]) - 1)) +
         (A2 >> (logg[bank] - bank));
   A = A1 ^ A2;
   if (bank < logg[bank])
      A =
         ((A << bank) & ((1 << logg[bank]) - 1)) + (A >> (logg[bank] - bank));
   return (A);
}

// gindex computes a full hash of PC, ghist and phist
int tagescl_t::gindex(unsigned int PC, int bank, long long hist,
                      folded_history *ch_i) {
   int index;
   int M = (m[bank] > PHISTWIDTH) ? PHISTWIDTH : m[bank];
   index =
      PC ^ (PC >> (abs(logg[bank] - bank) + 1)) ^ ch_i[bank].comp ^ F(hist, M, bank);

   return (index & ((1 << (logg[bank])) - 1));
}

//  tag computation
uint16_t tagescl_t::gtag(unsigned int PC, int bank, folded_history *ch0,
                         folded_history *ch1) {
   int tag = (PC) ^ ch0[bank].comp ^ (ch1[bank].comp << 1);
   return (tag & ((1 << (TB[bank])) - 1));
}

// up-down saturating counter
void tagescl_t::ctrupdate(int8_t &ctr, bool taken, int nbits) {
   if (taken) {
      if (ctr < ((1 << (nbits - 1)) - 1))
         ctr++;
   }
   else {
      if (ctr > -(1 << (nbits - 1)))
         ctr--;
   }
}

bool tagescl_t::getconf() {
   return HighConf;
}

void tagescl_t::getPredInfo(int &TageBimIndex, int &TageBimPred, int TageIndex[], uint TageTag[],
                            bool &TagePredTaken, bool &TagePred, bool &TageAltPred, bool &TageLongestMatchPred,
                            int &TageHitBank, int &TageAltBank, bool &TageAltConf, long long &TagePhist, int &TagePTGhist, uint8_t TageHist[], int &TageSeed,
                            bool &Tagepredloop, int &TageLIB, int &TageLI, int &TageLHIT, int &TageLTAG, bool &TageLVALID,
                            bool &TagePredInter, int &TageLSUM, bool &TageHighConf, bool &TageMedConf, bool &TageLowConf, long long &TageGHIST, int &TageTHRES,
                            long long &TageIMLIcount, long long TageIMHIST[], long long TageL_shist[], long long TageS_slhist[], long long TageT_slhist[], folded_history TageCh_i[], folded_history TageCh_t0[], folded_history TageCh_t1[]) {

   TageBimIndex = BI;
   TageBimPred = BIM;
   for (int i = 0; i < 37; i++) {
      TageIndex[i] = GI[i];
      TageTag[i] = GTAG[i];
   }
   TagePredTaken = pred_taken;
   TagePred = tage_pred;
   TageAltPred = alttaken;
   TageLongestMatchPred = LongestMatchPred;
   TageHitBank = HitBank;
   TageAltBank = AltBank;
   TageAltConf = AltConf;
   TagePhist = phist;
   TagePTGhist = ptghist;
   //TageSeed = Seed;
   for (int i = 0; i < 4096; i++) {
      TageHist[i] = ghist[i];
   }
#ifdef LOOPPREDICTOR
   Tagepredloop = predloop;
   TageLIB = LIB;
   TageLI = LI;
   TageLHIT = LHIT;
   TageLTAG = LTAG;
   TageLVALID = LVALID;
#endif
#ifdef SC
   TagePredInter = pred_inter;
   TageLSUM = LSUM;
   TageHighConf = HighConf;
   TageMedConf = MedConf;
   TageLowConf = LowConf;
   TageGHIST = GHIST;
   TageTHRES = THRES;
#ifdef IMLI
   TageIMLIcount = IMLIcount;
   for (int i = 0; i < 256; i++)
      TageIMHIST[i] = IMHIST[i];
#endif
#ifdef LOCALH
   for (int i = 0; i < NLOCAL; i++)
      TageL_shist[i] = L_shist[i];
   for (int i = 0; i < NSECLOCAL; i++)
      TageS_slhist[i] = S_slhist[i];
   for (int i = 0; i < NTLOCAL; i++)
      TageT_slhist[i] = T_slhist[i];
#endif
#endif
   for (int i = 1; i < 37; i++) {
      TageCh_i[i] = ch_i[i];
      TageCh_t0[i] = ch_t[0][i];
      TageCh_t1[i] = ch_t[1][i];
   }
}

void tagescl_t::getTageHist(long long &TagePhist, int &TagePTGhist, uint8_t TageHist[], long long &TageGHIST,
                            long long &TageIMLIcount, long long TageIMHIST[], long long TageL_shist[], long long TageS_slhist[], long long TageT_slhist[],
                            folded_history TageCh_i[], folded_history TageCh_t0[], folded_history TageCh_t1[], int &TageSeed) {
   TagePhist = phist;
   TagePTGhist = ptghist;
   for (int i = 0; i < 4096; i++) {
      TageHist[i] = ghist[i];
   }
#ifdef SC
   TageGHIST = GHIST;
#ifdef IMLI
   TageIMLIcount = IMLIcount;
   for (int i = 0; i < 256; i++)
      TageIMHIST[i] = IMHIST[i];
#endif
#ifdef LOCALH
   for (int i = 0; i < NLOCAL; i++)
      TageL_shist[i] = L_shist[i];
   for (int i = 0; i < NSECLOCAL; i++)
      TageS_slhist[i] = S_slhist[i];
   for (int i = 0; i < NTLOCAL; i++)
      TageT_slhist[i] = T_slhist[i];
#endif
#endif
   for (int i = 1; i < 37; i++) {
      TageCh_i[i] = ch_i[i];
      TageCh_t0[i] = ch_t[0][i];
      TageCh_t1[i] = ch_t[1][i];
   }
   //TageSeed = Seed;
}

bool tagescl_t::getbim() {
   BIM = (btable[BI].pred << 1) + (btable[BI >> HYSTSHIFT].hyst);
   HighConf = (BIM == 0) || (BIM == 3);
   LowConf = !HighConf;
   AltConf = HighConf;
   MedConf = false;
   return (btable[BI].pred > 0);
}

void tagescl_t::baseupdate(bool Taken) {
   int inter = BIM;
   if (Taken) {
      if (inter < 3)
         inter += 1;
   }
   else if (inter > 0)
      inter--;
   btable[BI].pred = inter >> 1;
   btable[BI >> HYSTSHIFT].hyst = (inter & 1);
}


void tagescl_t::baseupdateMicro(bool Taken, int TageBimIndex, int TageBimPred) {
   int inter = TageBimPred;
   if (Taken) {
      if (inter < 3)
         inter += 1;
   }
   else if (inter > 0)
      inter--;
   btable[TageBimIndex].pred = inter >> 1;
   btable[TageBimIndex >> HYSTSHIFT].hyst = (inter & 1);
}
//just a simple pseudo random number generator: use available information
// to allocate entries  in the loop predictor
int tagescl_t::MYRANDOM() {
   Seed++;
   Seed ^= phist;
   Seed = (Seed >> 21) + (Seed << 11);
   Seed ^= ptghist;
   Seed = (Seed >> 10) + (Seed << 22);
   return (Seed);
}

int tagescl_t::MYRANDOMMicro(int TageSeed, long long TagePhist, int TagePTGhist) {
   TageSeed++;
   TageSeed ^= TagePhist;
   TageSeed = (TageSeed >> 21) + (TageSeed << 11);
   TageSeed ^= TagePTGhist;
   TageSeed = (TageSeed >> 10) + (TageSeed << 22);
   return (TageSeed);
}

//  TAGE PREDICTION: same code at fetch or retire time but the index and tags must recomputed
void tagescl_t::Tagepred(UINT64 PC) {
   HitBank = 0;
   AltBank = 0;
   for (int i = 1; i <= NHIST; i += 2) {
      GI[i] = gindex(PC, i, phist, ch_i);
      GTAG[i] = gtag(PC, i, ch_t[0], ch_t[1]);
      GTAG[i + 1] = GTAG[i];
      GI[i + 1] = GI[i] ^ (GTAG[i] & ((1 << LOGG) - 1));
   }
   int T = (PC ^ (phist & ((1 << m[BORN]) - 1))) % NBANKHIGH;
   //int T = (PC ^ phist) % NBANKHIGH;
   for (int i = BORN; i <= NHIST; i++)
      if (NOSKIP[i]) {
         GI[i] += (T << LOGG);
         T++;
         T = T % NBANKHIGH;
      }
   T = (PC ^ (phist & ((1 << m[1]) - 1))) % NBANKLOW;

   for (int i = 1; i <= BORN - 1; i++)
      if (NOSKIP[i]) {
         GI[i] += (T << LOGG);
         T++;
         T = T % NBANKLOW;
      }
   //just do not forget most address are aligned on 4 bytes
   BI = (PC ^ (PC >> 2)) & ((1 << LOGB) - 1);

   {
      alttaken = getbim();
      tage_pred = alttaken;
      LongestMatchPred = alttaken;
   }

   //Look for the bank with longest matching history
   for (int i = NHIST; i > 0; i--) {
      if (NOSKIP[i])
         if (gtable[i][GI[i]].tag == GTAG[i]) {
            HitBank = i;
            LongestMatchPred = (gtable[HitBank][GI[HitBank]].ctr >= 0);
            break;
         }
   }

   //Look for the alternate bank
   for (int i = HitBank - 1; i > 0; i--) {
      if (NOSKIP[i])
         if (gtable[i][GI[i]].tag == GTAG[i]) {

            AltBank = i;
            break;
         }
   }
   //computes the prediction and the alternate prediction

   if (HitBank > 0) {
      if (AltBank > 0) {
         alttaken = (gtable[AltBank][GI[AltBank]].ctr >= 0);
         AltConf = (abs(2 * gtable[AltBank][GI[AltBank]].ctr + 1) > 1);
      }
      else
         alttaken = getbim();

      //if the entry is recognized as a newly allocated entry and
      //USE_ALT_ON_NA is positive  use the alternate prediction

      bool Huse_alt_on_na = (use_alt_on_na[INDUSEALT] >= 0);
      if ((!Huse_alt_on_na) || (abs(2 * gtable[HitBank][GI[HitBank]].ctr + 1) > 1))
         tage_pred = LongestMatchPred;
      else
         tage_pred = alttaken;

      HighConf =
         (abs(2 * gtable[HitBank][GI[HitBank]].ctr + 1) >=
          (1 << CWIDTH) - 1);
      LowConf = (abs(2 * gtable[HitBank][GI[HitBank]].ctr + 1) == 1);
      MedConf = (abs(2 * gtable[HitBank][GI[HitBank]].ctr + 1) == 5);
   }
}

//compute the prediction
bool tagescl_t::getPrediction(UINT64 PC) {
   // computes the TAGE table addresses and the partial tags

   Tagepred(PC);
   pred_taken = tage_pred;
#ifndef SC
   return (tage_pred);
#endif

#ifdef LOOPPREDICTOR
   predloop = getloop(PC); // loop prediction
   pred_taken = ((WITHLOOP >= 0) && (LVALID)) ? predloop : pred_taken;
#endif
   pred_inter = pred_taken;

   //Compute the SC prediction

   LSUM = 0;

   //integrate BIAS prediction
   int8_t ctr = Bias[INDBIAS];

   LSUM += (2 * ctr + 1);
   ctr = BiasSK[INDBIASSK];
   LSUM += (2 * ctr + 1);
   ctr = BiasBank[INDBIASBANK];
   LSUM += (2 * ctr + 1);
#ifdef VARTHRES
   LSUM = (1 + (WB[INDUPDS] >= 0)) * LSUM;
#endif
   //integrate the GEHL predictions
   LSUM +=
      Gpredict((PC << 1) + pred_inter, GHIST, Gm, GGEHL, GNB, LOGGNB, WG);
   LSUM += Gpredict(PC, phist, Pm, PGEHL, PNB, LOGPNB, WP);
#ifdef LOCALH
   LSUM += Gpredict(PC, L_shist[INDLOCAL], Lm, LGEHL, LNB, LOGLNB, WL);
#ifdef LOCALS
   LSUM += Gpredict(PC, S_slhist[INDSLOCAL], Sm, SGEHL, SNB, LOGSNB, WS);
#endif
#ifdef LOCALT
   LSUM += Gpredict(PC, T_slhist[INDTLOCAL], Tm, TGEHL, TNB, LOGTNB, WT);
#endif
#endif

#ifdef IMLI
   LSUM +=
      Gpredict(PC, IMHIST[(IMLIcount)], IMm, IMGEHL, IMNB, LOGIMNB, WIM);
   LSUM += Gpredict(PC, IMLIcount, Im, IGEHL, INB, LOGINB, WI);
#endif
   bool SCPRED = (LSUM >= 0);
   //just  an heuristic if the respective contribution of component groups can be multiplied by 2 or not
   THRES = (updatethreshold >> 3) + Pupdatethreshold[INDUPD]
#ifdef VARTHRES
           + 12 * ((WB[INDUPDS] >= 0) + (WP[INDUPDS] >= 0)
#ifdef LOCALH
                   + (WS[INDUPDS] >= 0) + (WT[INDUPDS] >= 0) + (WL[INDUPDS] >= 0)
#endif
                   + (WG[INDUPDS] >= 0)
#ifdef IMLI
                   + (WI[INDUPDS] >= 0)
#endif
                  )
#endif
      ;

   //Minimal benefit in trying to avoid accuracy loss on low confidence SC prediction and  high/medium confidence on TAGE
   // but just uses 2 counters 0.3 % MPKI reduction
   if (pred_inter != SCPRED) {
      //Choser uses TAGE confidence and |LSUM|
      pred_taken = SCPRED;
      if (HighConf) {
         if ((abs(LSUM) < THRES / 4)) {
            pred_taken = pred_inter;
         }

         else if ((abs(LSUM) < THRES / 2))
            pred_taken = (SecondH < 0) ? SCPRED : pred_inter;
      }

      if (MedConf)
         if ((abs(LSUM) < THRES / 4)) {
            pred_taken = (FirstH < 0) ? SCPRED : pred_inter;
         }
   }
   return pred_taken;
}

void tagescl_t::HistoryUpdate(UINT64 PC, OpType opType, bool taken,
                              UINT64 target, long long &X, int &Y,
                              folded_history *H, folded_history *G,
                              folded_history *J) {
   int brtype = 0;

   switch (opType) {
   case OPTYPE_RET_UNCOND:
   case OPTYPE_JMP_INDIRECT_UNCOND:
   case OPTYPE_JMP_INDIRECT_COND:
   case OPTYPE_CALL_INDIRECT_UNCOND:
   case OPTYPE_CALL_INDIRECT_COND:
   case OPTYPE_RET_COND:
      brtype = 2;
      break;
   case OPTYPE_JMP_DIRECT_COND:
   case OPTYPE_CALL_DIRECT_COND:
   case OPTYPE_JMP_DIRECT_UNCOND:
   case OPTYPE_CALL_DIRECT_UNCOND:
      brtype = 0;
      break;
   default:
      exit(1);
   }
   switch (opType) {
   case OPTYPE_JMP_DIRECT_COND:
   case OPTYPE_CALL_DIRECT_COND:
   case OPTYPE_JMP_INDIRECT_COND:
   case OPTYPE_CALL_INDIRECT_COND:
   case OPTYPE_RET_COND:
      brtype += 1;
      break;

   default:
      exit(1);
   }

   //special treatment for indirect  branchs;
   int maxt = 2;
   if (brtype & 1)
      maxt = 2;
   else if ((brtype & 2))
      maxt = 3;

#ifdef IMLI
   if (brtype & 1) {
#ifdef IMLI
      IMHIST[IMLIcount] = (IMHIST[IMLIcount] << 1) + taken;
#endif
      if (target < PC)

      {
         //This branch corresponds to a loop
         if (!taken) {
            //exit of the "loop"
            IMLIcount = 0;
         }
         if (taken) {

            if (IMLIcount < ((1 << Im[0]) - 1))
               IMLIcount++;
         }
      }
   }

#endif

   if (brtype & 1) {
      GHIST = (GHIST << 1) + (taken & (target < PC));
      L_shist[INDLOCAL] = (L_shist[INDLOCAL] << 1) + (taken);
      S_slhist[INDSLOCAL] =
         ((S_slhist[INDSLOCAL] << 1) + taken) ^ (PC & 15);
      T_slhist[INDTLOCAL] = (T_slhist[INDTLOCAL] << 1) + taken;
   }

   int T = ((PC ^ (PC >> 2))) ^ taken;
   int PATH = PC ^ (PC >> 2) ^ (PC >> 4);
   if ((brtype == 3) & taken) {
      T = (T ^ (target >> 2));
      PATH = PATH ^ (target >> 2) ^ (target >> 4);
   }

   for (int t = 0; t < maxt; t++) {
      bool DIR = (T & 1);
      T >>= 1;
      int PATHBIT = (PATH & 127);
      PATH >>= 1;
      //update  history
      Y--;
      ghist[Y & (HISTBUFFERLENGTH - 1)] = DIR;
      X = (X << 1) ^ PATHBIT;

      for (int i = 1; i <= NHIST; i++) {

         H[i].update(ghist, Y);
         G[i].update(ghist, Y);
         J[i].update(ghist, Y);
      }
   }

   X = (X & ((1 << PHISTWIDTH) - 1));
}

void tagescl_t::HistoryRevertAndUpdate(UINT64 PC, OpType opType, bool taken,
                                       UINT64 target, long long TagePhist, int TagePTGhist, uint8_t TageHist[],
                                       long long TageGHIST, long long TageIMLIcount, long long TageIMHIST[],
                                       long long TageL_shist[], long long TageS_slhist[], long long TageT_slhist[],
                                       folded_history TageCh_i[], folded_history TageCh_t0[], folded_history TageCh_t1[]) {
   //Reverting State
   phist = TagePhist;
   ptghist = TagePTGhist;
   GHIST = TageGHIST;
   IMLIcount = TageIMLIcount;
   for (int i = 0; i < 4096; i++)
      ghist[i] = TageHist[i];
#ifdef IMLI
   for (int i = 0; i < 256; i++)
      IMHIST[i] = TageIMHIST[i];
#endif
#ifdef SC
   for (int i = 0; i < NLOCAL; i++)
      L_shist[i] = TageL_shist[i];
   for (int i = 0; i < NSECLOCAL; i++)
      S_slhist[i] = TageS_slhist[i];
   for (int i = 0; i < NTLOCAL; i++)
      T_slhist[i] = TageT_slhist[i];
#endif
   for (int i = 1; i < 37; i++) {
      ch_i[i] = TageCh_i[i];
      ch_t[0][i] = TageCh_t0[i];
      ch_t[1][i] = TageCh_t1[i];
   }

   int brtype = 0;

   switch (opType) {
   case OPTYPE_RET_UNCOND:
   case OPTYPE_JMP_INDIRECT_UNCOND:
   case OPTYPE_JMP_INDIRECT_COND:
   case OPTYPE_CALL_INDIRECT_UNCOND:
   case OPTYPE_CALL_INDIRECT_COND:
   case OPTYPE_RET_COND:
      brtype = 2;
      break;
   case OPTYPE_JMP_DIRECT_COND:
   case OPTYPE_CALL_DIRECT_COND:
   case OPTYPE_JMP_DIRECT_UNCOND:
   case OPTYPE_CALL_DIRECT_UNCOND:
      brtype = 0;
      break;
   default:
      exit(1);
   }
   switch (opType) {
   case OPTYPE_JMP_DIRECT_COND:
   case OPTYPE_CALL_DIRECT_COND:
   case OPTYPE_JMP_INDIRECT_COND:
   case OPTYPE_CALL_INDIRECT_COND:
   case OPTYPE_RET_COND:
      brtype += 1;
      break;

   default:
      exit(1);
   }

   //special treatment for indirect  branchs;
   int maxt = 2;
   if (brtype & 1)
      maxt = 2;
   else if ((brtype & 2))
      maxt = 3;

#ifdef IMLI
   if (brtype & 1) {
#ifdef IMLI
      IMHIST[IMLIcount] = (IMHIST[IMLIcount] << 1) + taken;
#endif
      if (target < PC)

      {
         //This branch corresponds to a loop
         if (!taken) {
            //exit of the "loop"
            IMLIcount = 0;
         }
         if (taken) {

            if (IMLIcount < ((1 << Im[0]) - 1))
               IMLIcount++;
         }
      }
   }

#endif

   if (brtype & 1) {
      GHIST = (GHIST << 1) + (taken & (target < PC));
      L_shist[INDLOCAL] = (L_shist[INDLOCAL] << 1) + (taken);
      S_slhist[INDSLOCAL] =
         ((S_slhist[INDSLOCAL] << 1) + taken) ^ (PC & 15);
      T_slhist[INDTLOCAL] = (T_slhist[INDTLOCAL] << 1) + taken;
   }

   int T = ((PC ^ (PC >> 2))) ^ taken;
   int PATH = PC ^ (PC >> 2) ^ (PC >> 4);
   if ((brtype == 3) & taken) {
      T = (T ^ (target >> 2));
      PATH = PATH ^ (target >> 2) ^ (target >> 4);
   }

   for (int t = 0; t < maxt; t++) {
      bool DIR = (T & 1);
      T >>= 1;
      int PATHBIT = (PATH & 127);
      PATH >>= 1;
      //update  history
      ptghist--;
      ghist[ptghist & (HISTBUFFERLENGTH - 1)] = DIR;
      phist = (phist << 1) ^ PATHBIT;

      for (int i = 1; i <= NHIST; i++) {

         ch_i[i].update(ghist, ptghist);
         ch_t[0][i].update(ghist, ptghist);
         ch_t[1][i].update(ghist, ptghist);
      }
   }

   phist = (phist & ((1 << PHISTWIDTH) - 1));
}


void tagescl_t::UpdatePredictor(UINT64 PC, OpType opType, bool resolveDir,
                                bool predDir, UINT64 branchTarget) {

#ifdef SC
#ifdef LOOPPREDICTOR
   if (LVALID) {
      if (pred_taken != predloop)
         ctrupdate(WITHLOOP, (predloop == resolveDir), 7);
   }
   loopupdate(PC, resolveDir, (pred_taken != resolveDir));
#endif

   bool SCPRED = (LSUM >= 0);
   if (pred_inter != SCPRED) {
      if ((abs(LSUM) < THRES))
         if ((HighConf)) {

            if ((abs(LSUM) < THRES / 2))
               if ((abs(LSUM) >= THRES / 4))
                  ctrupdate(SecondH, (pred_inter == resolveDir), CONFWIDTH);
         }
      if ((MedConf))
         if ((abs(LSUM) < THRES / 4)) {
            ctrupdate(FirstH, (pred_inter == resolveDir), CONFWIDTH);
         }
   }

   if ((SCPRED != resolveDir) || ((abs(LSUM) < THRES))) {
      {
         if (SCPRED != resolveDir) {
            Pupdatethreshold[INDUPD] += 1;
            updatethreshold += 1;
         }

         else {
            Pupdatethreshold[INDUPD] -= 1;
            updatethreshold -= 1;
         }

         if (Pupdatethreshold[INDUPD] >= (1 << (WIDTHRESP - 1)))
            Pupdatethreshold[INDUPD] = (1 << (WIDTHRESP - 1)) - 1;
         //Pupdatethreshold[INDUPD] could be negative
         if (Pupdatethreshold[INDUPD] < -(1 << (WIDTHRESP - 1)))
            Pupdatethreshold[INDUPD] = -(1 << (WIDTHRESP - 1));
         if (updatethreshold >= (1 << (WIDTHRES - 1)))
            updatethreshold = (1 << (WIDTHRES - 1)) - 1;
         //updatethreshold could be negative
         if (updatethreshold < -(1 << (WIDTHRES - 1)))
            updatethreshold = -(1 << (WIDTHRES - 1));
      }
#ifdef VARTHRES
      {
         int XSUM =
            LSUM - ((WB[INDUPDS] >= 0) * ((2 * Bias[INDBIAS] + 1) +
                                          (2 * BiasSK[INDBIASSK] + 1) +
                                          (2 * BiasBank[INDBIASBANK] + 1)));
         if ((XSUM +
                 ((2 * Bias[INDBIAS] + 1) + (2 * BiasSK[INDBIASSK] + 1) +
                  (2 * BiasBank[INDBIASBANK] + 1)) >=
              0) != (XSUM >= 0))
            ctrupdate(WB[INDUPDS],
                      (((2 * Bias[INDBIAS] + 1) +
                           (2 * BiasSK[INDBIASSK] + 1) +
                           (2 * BiasBank[INDBIASBANK] + 1) >=
                        0) == resolveDir),
                      EWIDTH);
      }
#endif
      ctrupdate(Bias[INDBIAS], resolveDir, PERCWIDTH);
      ctrupdate(BiasSK[INDBIASSK], resolveDir, PERCWIDTH);
      ctrupdate(BiasBank[INDBIASBANK], resolveDir, PERCWIDTH);
      Gupdate((PC << 1) + pred_inter, resolveDir,
              GHIST, Gm, GGEHL, GNB, LOGGNB, WG);
      Gupdate(PC, resolveDir, phist, Pm, PGEHL, PNB, LOGPNB, WP);
#ifdef LOCALH
      Gupdate(PC, resolveDir, L_shist[INDLOCAL], Lm, LGEHL, LNB, LOGLNB,
              WL);
#ifdef LOCALS
      Gupdate(PC, resolveDir, S_slhist[INDSLOCAL], Sm,
              SGEHL, SNB, LOGSNB, WS);
#endif
#ifdef LOCALT

      Gupdate(PC, resolveDir, T_slhist[INDTLOCAL], Tm, TGEHL, TNB, LOGTNB,
              WT);
#endif
#endif

#ifdef IMLI
      Gupdate(PC, resolveDir, IMHIST[(IMLIcount)], IMm, IMGEHL, IMNB,
              LOGIMNB, WIM);
      Gupdate(PC, resolveDir, IMLIcount, Im, IGEHL, INB, LOGINB, WI);
#endif
   }
#endif

   //TAGE UPDATE
   bool ALLOC = ((tage_pred != resolveDir) & (HitBank < NHIST));

   //do not allocate too often if the overall prediction is correct

   if (HitBank > 0) {
      // Manage the selection between longest matching and alternate matching
      // for "pseudo"-newly allocated longest matching entry
      // this is extremely important for TAGE only, not that important when the overall predictor is implemented
      bool PseudoNewAlloc =
         (abs(2 * gtable[HitBank][GI[HitBank]].ctr + 1) <= 1);
      // an entry is considered as newly allocated if its prediction counter is weak
      if (PseudoNewAlloc) {
         if (LongestMatchPred == resolveDir)
            ALLOC = false;
         // if it was delivering the correct prediction, no need to allocate a new entry
         //even if the overall prediction was false

         if (LongestMatchPred != alttaken) {
            ctrupdate(use_alt_on_na[INDUSEALT], (alttaken == resolveDir),
                      ALTWIDTH);
         }
      }
   }

   if (pred_taken == resolveDir)
      if ((MYRANDOM() & 31) != 0)
         ALLOC = false;

   if (ALLOC) {

      int T = NNN;

      int A = 1;
      if ((MYRANDOM() & 127) < 32)
         A = 2;
      int Penalty = 0;
      int NA = 0;
      int DEP = ((((HitBank - 1 + 2 * A) & 0xffe)) ^ (MYRANDOM() & 1));
      // just a complex formula to chose between X and X+1, when X is odd: sorry

      for (int I = DEP; I < NHIST; I += 2) {
         int i = I + 1;
         bool Done = false;
         if (NOSKIP[i]) {
            if (gtable[i][GI[i]].u == 0)

            {
#define OPTREMP
// the replacement is optimized with a single u bit: 0.2 %
#ifdef OPTREMP
               if (abs(2 * gtable[i][GI[i]].ctr + 1) <= 3)
#endif
               {
                  gtable[i][GI[i]].tag = GTAG[i];
                  gtable[i][GI[i]].ctr = (resolveDir) ? 0 : -1;
                  NA++;
                  if (T <= 0) {
                     break;
                  }
                  I += 2;
                  Done = true;
                  T -= 1;
               }
#ifdef OPTREMP
               else {
                  if (gtable[i][GI[i]].ctr > 0)
                     gtable[i][GI[i]].ctr--;
                  else
                     gtable[i][GI[i]].ctr++;
               }

#endif
            }

            else {
               Penalty++;
            }
         }

         if (!Done) {
            i = (I ^ 1) + 1;
            if (NOSKIP[i]) {

               if (gtable[i][GI[i]].u == 0) {
#ifdef OPTREMP
                  if (abs(2 * gtable[i][GI[i]].ctr + 1) <= 3)
#endif

                  {
                     gtable[i][GI[i]].tag = GTAG[i];
                     gtable[i][GI[i]].ctr = (resolveDir) ? 0 : -1;
                     NA++;
                     if (T <= 0) {
                        break;
                     }
                     I += 2;
                     T -= 1;
                  }
#ifdef OPTREMP
                  else {
                     if (gtable[i][GI[i]].ctr > 0)
                        gtable[i][GI[i]].ctr--;
                     else
                        gtable[i][GI[i]].ctr++;
                  }

#endif
               }
               else {
                  Penalty++;
               }
            }
         }
      }
      TICK += (Penalty - 2 * NA);

      //just the best formula for the Championship:
      //In practice when one out of two entries are useful
      if (TICK < 0)
         TICK = 0;
      if (TICK >= BORNTICK) {

         for (int i = 1; i <= BORN; i += BORN - 1)
            for (int j = 0; j < SizeTable[i]; j++)
               gtable[i][j].u >>= 1;
         TICK = 0;
      }
   }

   //update predictions
   if (HitBank > 0) {
      if (abs(2 * gtable[HitBank][GI[HitBank]].ctr + 1) == 1)
         if (LongestMatchPred != resolveDir)

         { // acts as a protection
            if (AltBank > 0) {
               ctrupdate(gtable[AltBank][GI[AltBank]].ctr,
                         resolveDir, CWIDTH);
            }
            if (AltBank == 0)
               baseupdate(resolveDir);
         }
      ctrupdate(gtable[HitBank][GI[HitBank]].ctr, resolveDir, CWIDTH);
      //sign changes: no way it can have been useful
      if (abs(2 * gtable[HitBank][GI[HitBank]].ctr + 1) == 1)
         gtable[HitBank][GI[HitBank]].u = 0;
      if (alttaken == resolveDir)
         if (AltBank > 0)
            if (abs(2 * gtable[AltBank][GI[AltBank]].ctr + 1) == 7)
               if (gtable[HitBank][GI[HitBank]].u == 1) {
                  if (LongestMatchPred == resolveDir) {
                     gtable[HitBank][GI[HitBank]].u = 0;
                  }
               }
   }

   else
      baseupdate(resolveDir);

   if (LongestMatchPred != alttaken)
      if (LongestMatchPred == resolveDir) {
         if (gtable[HitBank][GI[HitBank]].u < (1 << UWIDTH) - 1)
            gtable[HitBank][GI[HitBank]].u++;
      }
   //END TAGE UPDATE

   HistoryUpdate(PC, opType, resolveDir, branchTarget,
                 phist, ptghist, ch_i, ch_t[0], ch_t[1]);

   //END PREDICTOR UPDATE
}
// PREDICTOR UPDATE

void tagescl_t::UpdatePredictorMicro(UINT64 PC, OpType opType, bool resolveDir,
                                     bool predDir, UINT64 branchTarget, int TageBimIndex, int TageBimPred, int TageIndex[],
                                     uint TageTag[], bool TagePredTaken, bool TagePred, bool TageAltPred, bool TageLongestMatchPred,
                                     int TageHitBank, int TageAltBank, bool TageAltConf, long long TagePhist, int TagePTGhist, int TageSeed,
                                     bool Tagepredloop, int TageLIB, int TageLI, int TageLHIT, int TageLTAG, bool TageLVALID,
                                     bool TagePredInter, int TageLSUM, bool TageHighConf, bool TageMedConf, bool TageLowConf,
                                     long long TageGHIST, int TageTHRES, long long TageIMLIcount, long long TageIMHIST[], long long TageL_shist[], long long TageS_slhist[],
                                     long long TageT_slhist[]) {

   //    if(DEBUG_BP)
   //     printf("TAGE Update pred: pc%x taken:%d TagePred:%d BimPred%d TageIndex:%d TageHitBank:%d TageAltBank:%d TageBimIndex:%d\n", PC, resolveDir, predDir, TageBimPred, TageIndex[TageHitBank], TageHitBank, TageAltBank, TageBimIndex);
#ifdef SC
#ifdef LOOPPREDICTOR
   if (TageLVALID) {
      if (TagePredTaken != Tagepredloop)
         ctrupdate(WITHLOOP, (Tagepredloop == resolveDir), 7);
   }
   loopupdateMicro(PC, resolveDir, (TagePredTaken != resolveDir), Tagepredloop, TageLIB, TageLI, TageLHIT, TageLTAG, TageLVALID);
#endif

   bool SCPRED = (TageLSUM >= 0);
   if (TagePredInter != SCPRED) {
      if ((abs(TageLSUM) < TageTHRES))
         if ((TageHighConf)) {

            if ((abs(TageLSUM) < TageTHRES / 2))
               if ((abs(TageLSUM) >= TageTHRES / 4))
                  ctrupdate(SecondH, (TagePredInter == resolveDir), CONFWIDTH);
         }
      if ((TageMedConf))
         if ((abs(TageLSUM) < TageTHRES / 4)) {
            ctrupdate(FirstH, (TagePredInter == resolveDir), CONFWIDTH);
         }
   }

   if ((SCPRED != resolveDir) || ((abs(TageLSUM) < TageTHRES))) {
      {
         if (SCPRED != resolveDir) {
            Pupdatethreshold[INDUPD] += 1;
            updatethreshold += 1;
         }

         else {
            Pupdatethreshold[INDUPD] -= 1;
            updatethreshold -= 1;
         }

         if (Pupdatethreshold[INDUPD] >= (1 << (WIDTHRESP - 1)))
            Pupdatethreshold[INDUPD] = (1 << (WIDTHRESP - 1)) - 1;
         //Pupdatethreshold[INDUPD] could be negative
         if (Pupdatethreshold[INDUPD] < -(1 << (WIDTHRESP - 1)))
            Pupdatethreshold[INDUPD] = -(1 << (WIDTHRESP - 1));
         if (updatethreshold >= (1 << (WIDTHRES - 1)))
            updatethreshold = (1 << (WIDTHRES - 1)) - 1;
         //updatethreshold could be negative
         if (updatethreshold < -(1 << (WIDTHRES - 1)))
            updatethreshold = -(1 << (WIDTHRES - 1));
      }
#ifdef VARTHRES
      {
         int XSUM =
            TageLSUM - ((WB[INDUPDS] >= 0) * ((2 * Bias[INDBIASMicro] + 1) +
                                              (2 * BiasSK[INDBIASSKMicro] + 1) +
                                              (2 * BiasBank[INDBIASBANKMicro] + 1)));
         if ((XSUM +
                 ((2 * Bias[INDBIASMicro] + 1) + (2 * BiasSK[INDBIASSKMicro] + 1) +
                  (2 * BiasBank[INDBIASBANKMicro] + 1)) >=
              0) != (XSUM >= 0))
            ctrupdate(WB[INDUPDS],
                      (((2 * Bias[INDBIASMicro] + 1) +
                           (2 * BiasSK[INDBIASSKMicro] + 1) +
                           (2 * BiasBank[INDBIASBANKMicro] + 1) >=
                        0) == resolveDir),
                      EWIDTH);
      }
#endif
      ctrupdate(Bias[INDBIASMicro], resolveDir, PERCWIDTH);
      ctrupdate(BiasSK[INDBIASSKMicro], resolveDir, PERCWIDTH);
      ctrupdate(BiasBank[INDBIASBANKMicro], resolveDir, PERCWIDTH);
      Gupdate((PC << 1) + TagePredInter, resolveDir,
              TageGHIST, Gm, GGEHL, GNB, LOGGNB, WG);
      Gupdate(PC, resolveDir, TagePhist, Pm, PGEHL, PNB, LOGPNB, WP);
#ifdef LOCALH
      Gupdate(PC, resolveDir, TageL_shist[INDLOCAL], Lm, LGEHL, LNB, LOGLNB,
              WL);
#ifdef LOCALS
      Gupdate(PC, resolveDir, TageS_slhist[INDSLOCAL], Sm,
              SGEHL, SNB, LOGSNB, WS);
#endif
#ifdef LOCALT

      Gupdate(PC, resolveDir, TageT_slhist[INDTLOCAL], Tm, TGEHL, TNB, LOGTNB,
              WT);
#endif
#endif

#ifdef IMLI
      Gupdate(PC, resolveDir, TageIMHIST[(TageIMLIcount)], IMm, IMGEHL, IMNB,
              LOGIMNB, WIM);
      Gupdate(PC, resolveDir, TageIMLIcount, Im, IGEHL, INB, LOGINB, WI);
#endif
   }
#endif

   //TAGE UPDATE
   bool ALLOC = ((TagePred != resolveDir) & (TageHitBank < NHIST));

   //do not allocate too often if the overall prediction is correct

   if (TageHitBank > 0) {
      // Manage the selection between longest matching and alternate matching
      // for "pseudo"-newly allocated longest matching entry
      // this is extremely important for TAGE only, not that important when the overall predictor is implemented
      bool PseudoNewAlloc =
         (abs(2 * gtable[TageHitBank][TageIndex[TageHitBank]].ctr + 1) <= 1);
      // an entry is considered as newly allocated if its prediction counter is weak
      if (PseudoNewAlloc) {
         if (TageLongestMatchPred == resolveDir)
            ALLOC = false;
         // if it was delivering the correct prediction, no need to allocate a new entry
         //even if the overall prediction was false

         int ind_use_alt = (((((TageHitBank - 1) / 8) << 1) + TageAltConf) % (SIZEUSEALT - 1));
         if (TageLongestMatchPred != TageAltPred) {
            ctrupdate(use_alt_on_na[ind_use_alt], (TageAltPred == resolveDir),
                      ALTWIDTH);
         }
      }
   }

   if (TagePredTaken == resolveDir)
      if ((MYRANDOMMicro(TageSeed, TagePhist, TagePTGhist) & 31) != 0)
         ALLOC = false;

   if (ALLOC) {

      int T = NNN;

      int A = 1;
      if ((MYRANDOMMicro(TageSeed, TagePhist, TagePTGhist) & 127) < 32)
         A = 2;
      int Penalty = 0;
      int NA = 0;
      int DEP = ((((TageHitBank - 1 + 2 * A) & 0xffe)) ^ (MYRANDOMMicro(TageSeed, TagePhist, TagePTGhist) & 1));
      // just a complex formula to chose between X and X+1, when X is odd: sorry

      for (int I = DEP; I < NHIST; I += 2) {
         int i = I + 1;
         bool Done = false;
         if (NOSKIP[i]) {
            if (gtable[i][TageIndex[i]].u == 0)

            {
#define OPTREMP
// the replacement is optimized with a single u bit: 0.2 %
#ifdef OPTREMP
               if (abs(2 * gtable[i][TageIndex[i]].ctr + 1) <= 3)
#endif
               {
                  gtable[i][TageIndex[i]].tag = TageTag[i];
                  gtable[i][TageIndex[i]].ctr = (resolveDir) ? 0 : -1;
                  NA++;
                  if (T <= 0) {
                     break;
                  }
                  I += 2;
                  Done = true;
                  T -= 1;
               }
#ifdef OPTREMP
               else {
                  if (gtable[i][TageIndex[i]].ctr > 0)
                     gtable[i][TageIndex[i]].ctr--;
                  else
                     gtable[i][TageIndex[i]].ctr++;
               }

#endif
            }

            else {
               Penalty++;
            }
         }

         if (!Done) {
            i = (I ^ 1) + 1;
            if (NOSKIP[i]) {

               if (gtable[i][TageIndex[i]].u == 0) {
#ifdef OPTREMP
                  if (abs(2 * gtable[i][TageIndex[i]].ctr + 1) <= 3)
#endif

                  {
                     gtable[i][TageIndex[i]].tag = TageTag[i];
                     gtable[i][TageIndex[i]].ctr = (resolveDir) ? 0 : -1;
                     NA++;
                     if (T <= 0) {
                        break;
                     }
                     I += 2;
                     T -= 1;
                  }
#ifdef OPTREMP
                  else {
                     if (gtable[i][TageIndex[i]].ctr > 0)
                        gtable[i][TageIndex[i]].ctr--;
                     else
                        gtable[i][TageIndex[i]].ctr++;
                  }

#endif
               }
               else {
                  Penalty++;
               }
            }
         }
      }
      TICK += (Penalty - 2 * NA);

      //just the best formula for the Championship:
      //In practice when one out of two entries are useful
      if (TICK < 0)
         TICK = 0;
      if (TICK >= BORNTICK) {

         for (int i = 1; i <= BORN; i += BORN - 1)
            for (int j = 0; j < SizeTable[i]; j++)
               gtable[i][j].u >>= 1;
         TICK = 0;
      }
   }

   //update predictions
   if (TageHitBank > 0) {
      if (abs(2 * gtable[TageHitBank][TageIndex[TageHitBank]].ctr + 1) == 1)
         if (TageLongestMatchPred != resolveDir)

         { // acts as a protection
            if (TageAltBank > 0) {
               ctrupdate(gtable[TageAltBank][TageIndex[TageAltBank]].ctr,
                         resolveDir, CWIDTH);
            }
            if (TageAltBank == 0)
               baseupdateMicro(resolveDir, TageBimIndex, TageBimPred);
         }
      ctrupdate(gtable[TageHitBank][TageIndex[TageHitBank]].ctr, resolveDir, CWIDTH);
      //sign changes: no way it can have been useful
      if (abs(2 * gtable[TageHitBank][TageIndex[TageHitBank]].ctr + 1) == 1)
         gtable[TageHitBank][TageIndex[TageHitBank]].u = 0;
      if (TageAltPred == resolveDir)
         if (TageAltBank > 0)
            if (abs(2 * gtable[TageAltBank][TageIndex[TageAltBank]].ctr + 1) == 7)
               if (gtable[TageHitBank][TageIndex[TageHitBank]].u == 1) {
                  if (TageLongestMatchPred == resolveDir) {
                     gtable[TageHitBank][TageIndex[TageHitBank]].u = 0;
                  }
               }
   }

   else
      baseupdateMicro(resolveDir, TageBimIndex, TageBimPred);

   if (TageLongestMatchPred != TageAltPred)
      if (TageLongestMatchPred == resolveDir) {
         if (gtable[TageHitBank][TageIndex[TageHitBank]].u < (1 << UWIDTH) - 1)
            gtable[TageHitBank][TageIndex[TageHitBank]].u++;
      }
   //END TAGE UPDATE
   //END PREDICTOR UPDATE
}

#define GINDEX (((long long) PC) ^ bhist ^ (bhist >> (8 - i)) ^ (bhist >> (16 - 2 * i)) ^ (bhist >> (24 - 3 * i)) ^ (bhist >> (32 - 3 * i)) ^ (bhist >> (40 - 4 * i))) & ((1 << (logs - (i >= (NBR - 2)))) - 1)
int tagescl_t::Gpredict(UINT64 PC, long long BHIST, int *length,
                        int8_t **tab, int NBR, int logs, int8_t *W) {
   int PERCSUM = 0;
   for (int i = 0; i < NBR; i++) {
      long long bhist = BHIST & ((long long) ((1 << length[i]) - 1));
      long long index = GINDEX;

      int8_t ctr = tab[i][index];

      PERCSUM += (2 * ctr + 1);
   }
#ifdef VARTHRES
   PERCSUM = (1 + (W[INDUPDS] >= 0)) * PERCSUM;
#endif
   return ((PERCSUM));
}
void tagescl_t::Gupdate(UINT64 PC, bool taken, long long BHIST, int *length,
                        int8_t **tab, int NBR, int logs, int8_t *W) {

   int PERCSUM = 0;

   for (int i = 0; i < NBR; i++) {
      long long bhist = BHIST & ((long long) ((1 << length[i]) - 1));
      long long index = GINDEX;

      PERCSUM += (2 * tab[i][index] + 1);
      ctrupdate(tab[i][index], taken, PERCWIDTH);
   }
#ifdef VARTHRES
   {
      int XSUM = LSUM - ((W[INDUPDS] >= 0)) * PERCSUM;
      if ((XSUM + PERCSUM >= 0) != (XSUM >= 0))
         ctrupdate(W[INDUPDS], ((PERCSUM >= 0) == taken), EWIDTH);
   }
#endif
}

void tagescl_t::TrackOtherInst(UINT64 PC, OpType opType, bool taken,
                               UINT64 branchTarget) {

   HistoryUpdate(PC, opType, taken, branchTarget, phist,
                 ptghist, ch_i, ch_t[0], ch_t[1]);
}

void tagescl_t::SpecHistoryUpdate(UINT64 PC, OpType opType, bool taken,
                                  UINT64 branchTarget) {

   HistoryUpdate(PC, opType, taken, branchTarget, phist,
                 ptghist, ch_i, ch_t[0], ch_t[1]);
}

#ifdef LOOPPREDICTOR
int tagescl_t::lindex(UINT64 PC) {
   return (((PC ^ (PC >> 2)) & ((1 << (LOGL - 2)) - 1)) << 2);
}

//loop prediction: only used if high confidence
//skewed associative 4-way
//At fetch time: speculative
#define CONFLOOP 15
bool tagescl_t::getloop(UINT64 PC) {
   LHIT = -1;

   LI = lindex(PC);
   LIB = ((PC >> (LOGL - 2)) & ((1 << (LOGL - 2)) - 1));
   LTAG = (PC >> (LOGL - 2)) & ((1 << 2 * LOOPTAG) - 1);
   LTAG ^= (LTAG >> LOOPTAG);
   LTAG = (LTAG & ((1 << LOOPTAG) - 1));

   for (int i = 0; i < 4; i++) {
      int index = (LI ^ ((LIB >> i) << 2)) + i;

      if (ltable[index].TAG == LTAG) {
         LHIT = i;
         LVALID = ((ltable[index].confid == CONFLOOP) || (ltable[index].confid * ltable[index].NbIter > 128));

         if (ltable[index].CurrentIter + 1 == ltable[index].NbIter)
            return (!(ltable[index].dir));
         return ((ltable[index].dir));
      }
   }

   LVALID = false;
   return (false);
}

void tagescl_t::loopupdate(UINT64 PC, bool Taken, bool ALLOC) {
   if (LHIT >= 0) {
      int index = (LI ^ ((LIB >> LHIT) << 2)) + LHIT;
      //already a hit
      if (LVALID) {
         if (Taken != predloop) {
            // free the entry
            ltable[index].NbIter = 0;
            ltable[index].age = 0;
            ltable[index].confid = 0;
            ltable[index].CurrentIter = 0;
            return;
         }
         else if ((predloop != tage_pred) || ((MYRANDOM() & 7) == 0))
            if (ltable[index].age < CONFLOOP)
               ltable[index].age++;
      }

      ltable[index].CurrentIter++;
      ltable[index].CurrentIter &= ((1 << WIDTHNBITERLOOP) - 1);
      //loop with more than 2** WIDTHNBITERLOOP iterations are not treated correctly; but who cares :-)
      if (ltable[index].CurrentIter > ltable[index].NbIter) {
         ltable[index].confid = 0;
         ltable[index].NbIter = 0;
         //treat like the 1st encounter of the loop
      }
      if (Taken != ltable[index].dir) {
         if (ltable[index].CurrentIter == ltable[index].NbIter) {
            if (ltable[index].confid < CONFLOOP)
               ltable[index].confid++;
            if (ltable[index].NbIter < 3)
            //just do not predict when the loop count is 1 or 2
            {
               // free the entry
               ltable[index].dir = Taken;
               ltable[index].NbIter = 0;
               ltable[index].age = 0;
               ltable[index].confid = 0;
            }
         }
         else {
            if (ltable[index].NbIter == 0) {
               // first complete nest;
               ltable[index].confid = 0;
               ltable[index].NbIter = ltable[index].CurrentIter;
            }
            else {
               //not the same number of iterations as last time: free the entry
               ltable[index].NbIter = 0;
               ltable[index].confid = 0;
            }
         }
         ltable[index].CurrentIter = 0;
      }
   }
   else if (ALLOC)

   {
      UINT64 X = MYRANDOM() & 3;

      if ((MYRANDOM() & 3) == 0)
         for (int i = 0; i < 4; i++) {
            int LHIT = (X + i) & 3;
            int index = (LI ^ ((LIB >> LHIT) << 2)) + LHIT;
            if (ltable[index].age == 0) {
               ltable[index].dir = !Taken;
               // most of mispredictions are on last iterations
               ltable[index].TAG = LTAG;
               ltable[index].NbIter = 0;
               ltable[index].age = 7;
               ltable[index].confid = 0;
               ltable[index].CurrentIter = 0;
               break;
            }
            else
               ltable[index].age--;
            break;
         }
   }
}

void tagescl_t::loopupdateMicro(UINT64 PC, bool Taken, bool ALLOC, bool Tagepredloop, int TageLIB, int TageLI, int TageLHIT, int TageLTAG, bool TageLVALID) {
   if (TageLHIT >= 0) {
      int index = (TageLI ^ ((TageLIB >> TageLHIT) << 2)) + TageLHIT;
      //already a hit
      if (TageLVALID) {
         if (Taken != Tagepredloop) {
            // free the entry
            ltable[index].NbIter = 0;
            ltable[index].age = 0;
            ltable[index].confid = 0;
            ltable[index].CurrentIter = 0;
            return;
         }
         else if ((Tagepredloop != tage_pred) || ((MYRANDOM() & 7) == 0))
            if (ltable[index].age < CONFLOOP)
               ltable[index].age++;
      }

      ltable[index].CurrentIter++;
      ltable[index].CurrentIter &= ((1 << WIDTHNBITERLOOP) - 1);
      //loop with more than 2** WIDTHNBITERLOOP iterations are not treated correctly; but who cares :-)
      if (ltable[index].CurrentIter > ltable[index].NbIter) {
         ltable[index].confid = 0;
         ltable[index].NbIter = 0;
         //treat like the 1st encounter of the loop
      }
      if (Taken != ltable[index].dir) {
         if (ltable[index].CurrentIter == ltable[index].NbIter) {
            if (ltable[index].confid < CONFLOOP)
               ltable[index].confid++;
            if (ltable[index].NbIter < 3)
            //just do not predict when the loop count is 1 or 2
            {
               // free the entry
               ltable[index].dir = Taken;
               ltable[index].NbIter = 0;
               ltable[index].age = 0;
               ltable[index].confid = 0;
            }
         }
         else {
            if (ltable[index].NbIter == 0) {
               // first complete nest;
               ltable[index].confid = 0;
               ltable[index].NbIter = ltable[index].CurrentIter;
            }
            else {
               //not the same number of iterations as last time: free the entry
               ltable[index].NbIter = 0;
               ltable[index].confid = 0;
            }
         }
         ltable[index].CurrentIter = 0;
      }
   }
   else if (ALLOC)

   {
      UINT64 X = MYRANDOM() & 3;

      if ((MYRANDOM() & 3) == 0)
         for (int i = 0; i < 4; i++) {
            int TageLHIT = (X + i) & 3;
            int index = (TageLI ^ ((TageLIB >> TageLHIT) << 2)) + TageLHIT;
            if (ltable[index].age == 0) {
               ltable[index].dir = !Taken;
               // most of mispredictions are on last iterations
               ltable[index].TAG = TageLTAG;
               ltable[index].NbIter = 0;
               ltable[index].age = 7;
               ltable[index].confid = 0;
               ltable[index].CurrentIter = 0;
               break;
            }
            else
               ltable[index].age--;
            break;
         }
   }
}

#endif

void tagescl_t::HistoryRevert(
   long long TagePhist, int TagePTGhist, uint8_t TageHist[],
   long long TageGHIST, long long TageIMLIcount, long long TageIMHIST[],
   long long TageL_shist[], long long TageS_slhist[], long long TageT_slhist[],
   folded_history TageCh_i[], folded_history TageCh_t0[], folded_history TageCh_t1[]) {
   //Reverting State
   phist = TagePhist;
   ptghist = TagePTGhist;
   GHIST = TageGHIST;
   IMLIcount = TageIMLIcount;
   for (int i = 0; i < 4096; i++)
      ghist[i] = TageHist[i];
#ifdef IMLI
   for (int i = 0; i < 256; i++)
      IMHIST[i] = TageIMHIST[i];
#endif
#ifdef SC
   for (int i = 0; i < NLOCAL; i++)
      L_shist[i] = TageL_shist[i];
   for (int i = 0; i < NSECLOCAL; i++)
      S_slhist[i] = TageS_slhist[i];
   for (int i = 0; i < NTLOCAL; i++)
      T_slhist[i] = TageT_slhist[i];
#endif
   for (int i = 1; i < 37; i++) {
      ch_i[i] = TageCh_i[i];
      ch_t[0][i] = TageCh_t0[i];
      ch_t[1][i] = TageCh_t1[i];
   }
}


void tagescl_t::MyHistoryUpdate(UINT64 PC, OpType opType, bool taken,
                                UINT64 target, long long &X, int &Y,
                                uint8_t MyHist[],
                                long long &MyGHIST, long long &MyIMLIcount, long long MyIMHIST[],
                                long long MyL_shist[], long long MyS_slhist[], long long MyT_slhist[],
                                folded_history MyCh_i[], folded_history MyCh_t0[], folded_history MyCh_t1[]) {

   int brtype = 0;

   switch (opType) {
   case OPTYPE_RET_UNCOND:
   case OPTYPE_JMP_INDIRECT_UNCOND:
   case OPTYPE_JMP_INDIRECT_COND:
   case OPTYPE_CALL_INDIRECT_UNCOND:
   case OPTYPE_CALL_INDIRECT_COND:
   case OPTYPE_RET_COND:
      brtype = 2;
      break;
   case OPTYPE_JMP_DIRECT_COND:
   case OPTYPE_CALL_DIRECT_COND:
   case OPTYPE_JMP_DIRECT_UNCOND:
   case OPTYPE_CALL_DIRECT_UNCOND:
      brtype = 0;
      break;
   default:
      exit(1);
   }
   switch (opType) {
   case OPTYPE_JMP_DIRECT_COND:
   case OPTYPE_CALL_DIRECT_COND:
   case OPTYPE_JMP_INDIRECT_COND:
   case OPTYPE_CALL_INDIRECT_COND:
   case OPTYPE_RET_COND:
      brtype += 1;
      break;

   default:
      exit(1);
   }

   //special treatment for indirect  branchs;
   int maxt = 2;
   if (brtype & 1)
      maxt = 2;
   else if ((brtype & 2))
      maxt = 3;

#ifdef IMLI
   if (brtype & 1) {
#ifdef IMLI
      MyIMHIST[MyIMLIcount] = (MyIMHIST[MyIMLIcount] << 1) + taken;
#endif
      if (target < PC)

      {
         //This branch corresponds to a loop
         if (!taken) {
            //exit of the "loop"
            MyIMLIcount = 0;
         }
         if (taken) {

            if (MyIMLIcount < ((1 << Im[0]) - 1))
               MyIMLIcount++;
         }
      }
   }

#endif

   if (brtype & 1) {
      MyGHIST = (MyGHIST << 1) + (taken & (target < PC));
      MyL_shist[INDLOCAL] = (MyL_shist[INDLOCAL] << 1) + (taken);
      MyS_slhist[INDSLOCAL] =
         ((MyS_slhist[INDSLOCAL] << 1) + taken) ^ (PC & 15);
      MyT_slhist[INDTLOCAL] = (MyT_slhist[INDTLOCAL] << 1) + taken;
   }

   int T = ((PC ^ (PC >> 2))) ^ taken;
   int PATH = PC ^ (PC >> 2) ^ (PC >> 4);
   if ((brtype == 3) & taken) {
      T = (T ^ (target >> 2));
      PATH = PATH ^ (target >> 2) ^ (target >> 4);
   }

   for (int t = 0; t < maxt; t++) {
      bool DIR = (T & 1);
      T >>= 1;
      int PATHBIT = (PATH & 127);
      PATH >>= 1;
      //update  history
      Y--;
      MyHist[Y & (HISTBUFFERLENGTH - 1)] = DIR;
      X = (X << 1) ^ PATHBIT;

      for (int i = 1; i <= NHIST; i++) {

         MyCh_i[i].update(MyHist, Y);
         MyCh_t0[i].update(MyHist, Y);
         MyCh_t1[i].update(MyHist, Y);
      }
   }

   X = (X & ((1 << PHISTWIDTH) - 1));
}

void tagescl_t::getPredictionContext(int &TageBimIndex, int &TageBimPred, int TageIndex[], uint TageTag[],
                                     bool &TagePredTaken, bool &TagePred, bool &TageAltPred, bool &TageAltConf, bool &TageLongestMatchPred,
                                     int &TageHitBank, int &TageAltBank, int &TageSeed,
                                     bool &Tagepredloop, int &TageLIB, int &TageLI, int &TageLHIT, int &TageLTAG, bool &TageLVALID,
                                     bool &TagePredInter, int &TageLSUM, int &Tageupdatethreshold, bool &TageHighConf, bool &TageMedConf, bool &TageLowConf, int &TageTHRES) {

   TageBimIndex = BI;
   TageBimPred = BIM;
   for (int i = 0; i < 37; i++) {
      TageIndex[i] = GI[i];
      TageTag[i] = GTAG[i];
   }
   TagePredTaken = pred_taken;
   TagePred = tage_pred;
   TageAltPred = alttaken;
   TageLongestMatchPred = LongestMatchPred;
   TageHitBank = HitBank;
   TageAltBank = AltBank;
   TageAltConf = AltConf;
   TageSeed = Seed;
#ifdef LOOPPREDICTOR
   Tagepredloop = predloop;
   TageLIB = LIB;
   TageLI = LI;
   TageLHIT = LHIT;
   TageLTAG = LTAG;
   TageLVALID = LVALID;
#endif
#ifdef SC
   TagePredInter = pred_inter;
   TageLSUM = LSUM;
   Tageupdatethreshold = updatethreshold;
   TageHighConf = HighConf;
   TageMedConf = MedConf;
   TageLowConf = LowConf;
   TageTHRES = THRES;
#endif
}

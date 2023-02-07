//This file has been edited from https://github.com/scipopt/scip/blob/c82767bf8f06a1e899d64c4d9ae4d1c78f6f911b/examples/CallableLibrary/src/spring.c

#include <stdio.h>
#include <math.h>
#include <sstream>

#include <scip/scip.h>
#include <scip/scipdefplugins.h>

/* Model parameters */

/** number of possible wire types */
#define nwires 11

/** diameters of available diameters (in) */
static const SCIP_Real diameters[] = { 0.207, 0.225, 0.244, 0.263, 0.283, 0.307, 0.331, 0.362, 0.394, 0.4375, 0.500 };

/** preload (lb) */
static const SCIP_Real preload = 300;

/** maximal working load (lb) */
static const SCIP_Real maxworkload = 1000;

/** maximal deflection (in) */
static const SCIP_Real maxdeflect = 6;

/** deflection from preload (in) */
static const SCIP_Real deflectpreload = 1.25;

/** maximal free length of spring (in) */
static const SCIP_Real maxfreelen = 14.0;

/** maximal coil diameter (in) */
static const SCIP_Real maxcoildiam = 3.0;

/** maximal shear stress */
static const SCIP_Real maxshearstress = 189000.0;

/** shear modulus of material */
static const SCIP_Real shearmod = 11500000.0;


/** sets up problem */
static
SCIP_RETCODE setupProblem(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_VAR* coil;        /* coil diameter */
   SCIP_VAR* wire;        /* wire diameter */
   SCIP_VAR* defl;        /* deflection */
   SCIP_VAR* ncoils;      /* number of coils (integer) */
   SCIP_VAR* const1;      /* a constant */
   SCIP_VAR* const2;      /* another constant */
   SCIP_VAR* volume;      /* total volume */
   SCIP_VAR* y[nwires];   /* wire choice (binary) */

   SCIP_EXPR* coilexpr;
   SCIP_EXPR* wireexpr;
   SCIP_EXPR* deflexpr;
   SCIP_EXPR* ncoilsexpr;
   SCIP_EXPR* const1expr;
   SCIP_EXPR* const2expr;
   SCIP_EXPR* volumeexpr;

   SCIP_CONS* voldef;
   SCIP_CONS* defconst1;
   SCIP_CONS* defconst2;
   SCIP_CONS* shear;
   SCIP_CONS* defdefl;
   SCIP_CONS* freel;
   SCIP_CONS* coilwidth;
   SCIP_CONS* defwire;
   SCIP_CONS* selectwire;

   char name[SCIP_MAXSTRLEN];
   int i;

   /* create empty problem */
   SCIP_CALL( SCIPcreateProbBasic(scip, "spring") );

   /* create variables */
   SCIP_CALL( SCIPcreateVarBasic(scip, &coil, "coildiam", 0.0, SCIPinfinity(scip), 0.0, SCIP_VARTYPE_CONTINUOUS) );
   SCIP_CALL( SCIPcreateVarBasic(scip, &wire, "wirediam", 0.0, SCIPinfinity(scip), 0.0, SCIP_VARTYPE_CONTINUOUS) );
   SCIP_CALL( SCIPcreateVarBasic(scip, &defl, "deflection", 0.0, SCIPinfinity(scip), 0.0, SCIP_VARTYPE_CONTINUOUS) );
   SCIP_CALL( SCIPcreateVarBasic(scip, &ncoils, "ncoils", 0.0, SCIPinfinity(scip), 0.0, SCIP_VARTYPE_INTEGER) );
   SCIP_CALL( SCIPcreateVarBasic(scip, &const1, "const1", 0.0, SCIPinfinity(scip), 0.0, SCIP_VARTYPE_CONTINUOUS) );
   SCIP_CALL( SCIPcreateVarBasic(scip, &const2, "const2", 0.0, SCIPinfinity(scip), 0.0, SCIP_VARTYPE_CONTINUOUS) );
   SCIP_CALL( SCIPcreateVarBasic(scip, &volume, "volume", 0.0, SCIPinfinity(scip), 1.0, SCIP_VARTYPE_CONTINUOUS) );
   for( i = 0; i < nwires; ++i )
   {
      (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "wire%d", i+1);
      SCIP_CALL( SCIPcreateVarBasic(scip, &y[i], name, 0.0, 1.0, 0.0, SCIP_VARTYPE_BINARY) );
   }

   /* set nonstandard variable bounds */
   SCIP_CALL( SCIPchgVarLb(scip, defl, deflectpreload / (maxworkload - preload)) );
   SCIP_CALL( SCIPchgVarUb(scip, defl, maxdeflect / preload) );

   /* add variables to problem */
   SCIP_CALL( SCIPaddVar(scip, coil) );
   SCIP_CALL( SCIPaddVar(scip, wire) );
   SCIP_CALL( SCIPaddVar(scip, defl) );
   SCIP_CALL( SCIPaddVar(scip, ncoils) );
   SCIP_CALL( SCIPaddVar(scip, const1) );
   SCIP_CALL( SCIPaddVar(scip, const2) );
   SCIP_CALL( SCIPaddVar(scip, volume) );
   for( i = 0; i < nwires; ++i )
   {
      SCIP_CALL( SCIPaddVar(scip, y[i]) );
   }

   /* create variable expressions */
   SCIP_CALL( SCIPcreateExprVar(scip, &coilexpr, coil, NULL, NULL) );
   SCIP_CALL( SCIPcreateExprVar(scip, &wireexpr, wire, NULL, NULL) );
   SCIP_CALL( SCIPcreateExprVar(scip, &deflexpr, defl, NULL, NULL) );
   SCIP_CALL( SCIPcreateExprVar(scip, &ncoilsexpr, ncoils, NULL, NULL) );
   SCIP_CALL( SCIPcreateExprVar(scip, &const1expr, const1, NULL, NULL) );
   SCIP_CALL( SCIPcreateExprVar(scip, &const2expr, const2, NULL, NULL) );
   SCIP_CALL( SCIPcreateExprVar(scip, &volumeexpr, volume, NULL, NULL) );

   /* nonlinear constraint voldef: PI/2 * (ncoils+2)*coil*wire^2 - volume == 0 */
   {
      SCIP_EXPR* exprs[3];
      SCIP_EXPR* powexpr;
      SCIP_EXPR* prodexpr;
      SCIP_EXPR* sumexpr;
      SCIP_EXPR* expr;
      SCIP_Real coefs[2];

      /* create wire^2 */
      SCIP_CALL( SCIPcreateExprPow(scip, &powexpr, wireexpr, 2.0, NULL, NULL) );

      /* create (ncoils+2) */
      SCIP_CALL( SCIPcreateExprSum(scip, &sumexpr, 1, &ncoilsexpr, NULL, 2.0, NULL, NULL) );

      /* create (ncoils+2)*coil*wire^2 */
      exprs[0] = sumexpr;
      exprs[1] = coilexpr;
      exprs[2] = powexpr;
      SCIP_CALL( SCIPcreateExprProduct(scip, &prodexpr, 3, exprs, 1.0, NULL, NULL) );

      /* create PI/2 * (ncoils+2)*coil*wire^2 - volume */
      exprs[0] = prodexpr;
      coefs[0] = M_PI_2;
      exprs[1] = volumeexpr;
      coefs[1] = -1.0;
      SCIP_CALL( SCIPcreateExprSum(scip, &expr, 2, exprs, coefs, 0.0, NULL, NULL) );

      /* create nonlinear constraint */
      SCIP_CALL( SCIPcreateConsBasicNonlinear(scip, &voldef, "voldef", expr, 0.0, 0.0) );

      /* release expressions */
      SCIP_CALL( SCIPreleaseExpr(scip, &expr) );
      SCIP_CALL( SCIPreleaseExpr(scip, &prodexpr) );
      SCIP_CALL( SCIPreleaseExpr(scip, &sumexpr) );
      SCIP_CALL( SCIPreleaseExpr(scip, &powexpr) );
   }

   /* nonlinear constraint defconst1: coil / wire - const1 == 0.0 */
   {
      SCIP_EXPR* exprs[2];
      SCIP_EXPR* powexpr;
      SCIP_EXPR* prodexpr;
      SCIP_EXPR* sumexpr;
      SCIP_Real coefs[2];

      /* create 1 / wire */
      SCIP_CALL( SCIPcreateExprPow(scip, &powexpr, wireexpr, -1.0, NULL, NULL) );

      /* create coil / wire */
      exprs[0] = coilexpr;
      exprs[1] = powexpr;
      SCIP_CALL( SCIPcreateExprProduct(scip, &prodexpr, 2, exprs, 1.0, NULL, NULL) );

      /* create coil / wire - const1 */
      exprs[0] = prodexpr;
      coefs[0] = 1.0;
      exprs[1] = const1expr;
      coefs[1] = -1.0;
      SCIP_CALL( SCIPcreateExprSum(scip, &sumexpr, 2, exprs, coefs, 0.0, NULL, NULL) );

      /* create nonlinear constraint */
      SCIP_CALL( SCIPcreateConsBasicNonlinear(scip, &defconst1, "defconst1", sumexpr, 0.0, 0.0) );

      /* release expressions */
      SCIP_CALL( SCIPreleaseExpr(scip, &sumexpr) );
      SCIP_CALL( SCIPreleaseExpr(scip, &prodexpr) );
      SCIP_CALL( SCIPreleaseExpr(scip, &powexpr) );
   }

   /* nonlinear constraint defconst2: (4.0 * const1 - 1.0) / (4.0 * const1 - 4.0) + 0.615 / const1 - const2 == 0.0 */
   {
      SCIP_EXPR* exprs[3];
      SCIP_EXPR* sumexpr1;
      SCIP_EXPR* sumexpr2;
      SCIP_EXPR* powexpr1;
      SCIP_EXPR* powexpr2;
      SCIP_EXPR* prodexpr;
      SCIP_EXPR* expr;
      SCIP_Real coefs[3];

      /* create (4.0 * const1 - 1.0) */
      coefs[0] = 4.0;
      SCIP_CALL( SCIPcreateExprSum(scip, &sumexpr1, 1, &const1expr, coefs, -1.0, NULL, NULL) );

      /* create (4.0 * const1 - 4.0) */
      coefs[0] = 4.0;
      SCIP_CALL( SCIPcreateExprSum(scip, &sumexpr2, 1, &const1expr, coefs, -4.0, NULL, NULL) );

      /* create 1 / (4.0 * const1 - 4.0) */
      SCIP_CALL( SCIPcreateExprPow(scip, &powexpr1, sumexpr2, -1.0, NULL, NULL) );

      /* create (4.0 * const1 - 1.0) / (4.0 * const1 - 4.0) */
      exprs[0] = sumexpr1;
      exprs[1] = powexpr1;
      SCIP_CALL( SCIPcreateExprProduct(scip, &prodexpr, 2, exprs, 1.0, NULL, NULL) );

      /* create 1 / const1 */
      SCIP_CALL( SCIPcreateExprPow(scip, &powexpr2, const1expr, -1.0, NULL, NULL) );

      /* create (4.0 * const1 - 1.0) / (4.0 * const1 - 4.0) + 0.615 / const1 - const2 */
      exprs[0] = prodexpr;
      coefs[0] = 1.0;
      exprs[1] = powexpr2;
      coefs[1] = 0.615;
      exprs[2] = const2expr;
      coefs[2] = -1.0;
      SCIP_CALL( SCIPcreateExprSum(scip, &expr, 3, exprs, coefs, 0.0, NULL, NULL) );

      /* create nonlinear constraint */
      SCIP_CALL( SCIPcreateConsBasicNonlinear(scip, &defconst2, "defconst2", expr, 0.0, 0.0) );

      /* release expressions */
      SCIP_CALL( SCIPreleaseExpr(scip, &expr) );
      SCIP_CALL( SCIPreleaseExpr(scip, &powexpr2) );
      SCIP_CALL( SCIPreleaseExpr(scip, &prodexpr) );
      SCIP_CALL( SCIPreleaseExpr(scip, &powexpr1) );
      SCIP_CALL( SCIPreleaseExpr(scip, &sumexpr2) );
      SCIP_CALL( SCIPreleaseExpr(scip, &sumexpr1) );
   }

   /* quadratic constraint shear: 8.0*maxworkload/PI * const1 * const2 - maxshearstress * wire^2 <= 0.0 */
   {
      SCIP_VAR* quadvars1[2] = {const1, wire};
      SCIP_VAR* quadvars2[2] = {const2, wire};
      SCIP_Real quadcoefs[2] = {8.0 * maxworkload / M_PI, -maxshearstress};

      /* create empty quadratic constraint with right-hand-side 0.0 */
      SCIP_CALL( SCIPcreateConsQuadraticNonlinear(scip, &shear, "shear", 0, NULL, NULL, 2, quadvars1, quadvars2, quadcoefs,
         -SCIPinfinity(scip), 0.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE) );
   }

   /* nonlinear constraint defdefl: 8.0/shearmod * ncoils * const1^3 / wire - defl == 0.0 */
   {
      SCIP_EXPR* exprs[3];
      SCIP_EXPR* prodexpr;
      SCIP_EXPR* powexpr1;
      SCIP_EXPR* powexpr2;
      SCIP_EXPR* expr;
      SCIP_Real coefs[3];

      /* create const1^3 */
      SCIP_CALL( SCIPcreateExprPow(scip, &powexpr1, const1expr, 3.0, NULL, NULL) );

      /* create 1 / wire */
      SCIP_CALL( SCIPcreateExprPow(scip, &powexpr2, wireexpr, -1.0, NULL, NULL) );

      /* create ncoils * const1^3 / wire */
      exprs[0] = ncoilsexpr;
      exprs[1] = powexpr1;
      exprs[2] = powexpr2;
      SCIP_CALL( SCIPcreateExprProduct(scip, &prodexpr, 3, exprs, 1.0, NULL, NULL) );

      /* create 8.0/shearmod * ncoils * const1^3 / wire - defl */
      exprs[0] = prodexpr;
      coefs[0] = 8.0 / shearmod;
      exprs[1] = deflexpr;
      coefs[1] = -1.0;
      SCIP_CALL( SCIPcreateExprSum(scip, &expr, 2, exprs, coefs, 0.0, NULL, NULL) );

      /* create nonlinear constraint */
      SCIP_CALL( SCIPcreateConsBasicNonlinear(scip, &defdefl, "defdefl", expr, 0.0, 0.0) );

      /* release expressions */
      SCIP_CALL( SCIPreleaseExpr(scip, &expr) );
      SCIP_CALL( SCIPreleaseExpr(scip, &prodexpr) );
      SCIP_CALL( SCIPreleaseExpr(scip, &powexpr2) );
      SCIP_CALL( SCIPreleaseExpr(scip, &powexpr1) );
   }

   /* quadratic constraint freel: maxworkload * defl + 1.05 * ncoils * wire + 2.1 * wire <= maxfreelen */
   {
      SCIP_VAR* linvars[2] = {defl, wire};
      SCIP_Real lincoefs[2] = {maxworkload, 2.1};
      SCIP_Real one05 = 1.05;

      /* create quadratic constraint maxworkload * defl + 1.05 * ncoils * wire <= maxfreelen */
      SCIP_CALL( SCIPcreateConsQuadraticNonlinear(scip, &freel, "freel", 2, linvars, lincoefs, 1, &ncoils, &wire, &one05,
         -SCIPinfinity(scip), maxfreelen, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE,
         FALSE, FALSE) );
   }

   /* linear constraint coilwidth: coil + wire <= maxcoildiam */
   {
      /* create empty linear constraint with right-hand-side maxcoildiam */
      SCIP_CALL( SCIPcreateConsBasicLinear(scip, &coilwidth, "coilwidth", 0, NULL, NULL, -SCIPinfinity(scip), maxcoildiam) );

      /* add linear term coil + wire */
      SCIP_CALL( SCIPaddCoefLinear(scip, coilwidth, coil, 1.0) );
      SCIP_CALL( SCIPaddCoefLinear(scip, coilwidth, wire, 1.0) );
   }

   /* linear constraint defwire: sum_i b[i]*y[i] - wire == 0.0 */
   {
      /* create linear constraint sum_i b[i]*y[i] == 0.0 */
      SCIP_CALL( SCIPcreateConsBasicLinear(scip, &defwire, "defwire", nwires, y, (SCIP_Real*)diameters, 0.0, 0.0) );

      /* add term -wire */
      SCIP_CALL( SCIPaddCoefLinear(scip, defwire, wire, -1.0) );
   }

   /* specialized linear constraint selectwire: sum_i y[i] == 1.0 */
   {
      SCIP_CALL( SCIPcreateConsBasicSetpart(scip, &selectwire, "selectwire", nwires, y) );
   }

   /* add constraints to problem */
   SCIP_CALL( SCIPaddCons(scip, voldef) );
   SCIP_CALL( SCIPaddCons(scip, defconst1) );
   SCIP_CALL( SCIPaddCons(scip, defconst2) );
   SCIP_CALL( SCIPaddCons(scip, shear) );
   SCIP_CALL( SCIPaddCons(scip, defdefl) );
   SCIP_CALL( SCIPaddCons(scip, freel) );
   SCIP_CALL( SCIPaddCons(scip, coilwidth) );
   SCIP_CALL( SCIPaddCons(scip, defwire) );
   SCIP_CALL( SCIPaddCons(scip, selectwire) );

   /* release variable expressions */
   SCIP_CALL( SCIPreleaseExpr(scip, &volumeexpr) );
   SCIP_CALL( SCIPreleaseExpr(scip, &const2expr) );
   SCIP_CALL( SCIPreleaseExpr(scip, &const1expr) );
   SCIP_CALL( SCIPreleaseExpr(scip, &ncoilsexpr) );
   SCIP_CALL( SCIPreleaseExpr(scip, &deflexpr) );
   SCIP_CALL( SCIPreleaseExpr(scip, &wireexpr) );
   SCIP_CALL( SCIPreleaseExpr(scip, &coilexpr) );

   /* release variables and constraints
    * the problem has them captured, and we do not require them anymore
    */
   SCIP_CALL( SCIPreleaseVar(scip, &coil) );
   SCIP_CALL( SCIPreleaseVar(scip, &wire) );
   SCIP_CALL( SCIPreleaseVar(scip, &defl) );
   SCIP_CALL( SCIPreleaseVar(scip, &ncoils) );
   SCIP_CALL( SCIPreleaseVar(scip, &const1) );
   SCIP_CALL( SCIPreleaseVar(scip, &const2) );
   SCIP_CALL( SCIPreleaseVar(scip, &volume) );
   for( i = 0; i < nwires; ++i )
   {
      SCIP_CALL( SCIPreleaseVar(scip, &y[i]) );
   }

   SCIP_CALL( SCIPreleaseCons(scip, &voldef) );
   SCIP_CALL( SCIPreleaseCons(scip, &defconst1) );
   SCIP_CALL( SCIPreleaseCons(scip, &defconst2) );
   SCIP_CALL( SCIPreleaseCons(scip, &shear) );
   SCIP_CALL( SCIPreleaseCons(scip, &defdefl) );
   SCIP_CALL( SCIPreleaseCons(scip, &freel) );
   SCIP_CALL( SCIPreleaseCons(scip, &coilwidth) );
   SCIP_CALL( SCIPreleaseCons(scip, &defwire) );
   SCIP_CALL( SCIPreleaseCons(scip, &selectwire) );

   return SCIP_OKAY;
}

class SCIPinstance
{
    SCIP* scip{nullptr};

public:

    class Log
    {
        SCIP* m_scip{nullptr};
        FILE* m_file{nullptr};
        typedef void(*log_function)(SCIP* instance, FILE* file, const char* formatstr, ...);
        log_function m_outputLog;
        std::stringstream m_output;

        Log(log_function outputLog, SCIP* instance, FILE* file)
            : m_scip(instance)
            , m_file(file)
            , m_outputLog(outputLog)
        {}

        friend class SCIPinstance;


    public:

        Log(Log&& other)
            : m_scip(other.m_scip)
            , m_file(other.m_file)
            , m_outputLog(other.m_outputLog)
            , m_output(std::move(other.m_output))
        {
            other.m_scip = nullptr;
        }

        std::ostream& operator<<(const std::stringstream& input)
        {
            return m_output << input.str();
        }

        std::ostream& operator<<(const std::string& input)
        {
            return m_output << input;
        }

        ~Log()
        {
            if (m_scip)
                m_outputLog(m_scip, m_file, "%s\n", m_output.str().c_str());
        }
    };


    SCIPinstance()
    {
        SCIP_CALL_ABORT(SCIPcreate(&scip));
        SCIP_CALL_ABORT(SCIPincludeDefaultPlugins(scip));
    }

    ~SCIPinstance()
    {
        SCIP_CALL_ABORT(SCIPfree(&scip));
    }

    SCIP* get()
    {
        return scip;
    }

    Log info(FILE* file = NULL)
    {
        return Log(&SCIPinfoMessage, scip, file);
    }
};

/** runs spring example */
static
SCIP_RETCODE runSpring(void)
{
   SCIPinstance scipSolver;

   scipSolver.info(); //new line
   scipSolver.info() << "************************************************";
   scipSolver.info() << "* Running Coil Compression Spring Design Model *";
   scipSolver.info() << "************************************************";
   scipSolver.info();

   SCIP_CALL( setupProblem(scipSolver.get()) );

   scipSolver.info() << "Original problem:";
   SCIP_CALL( SCIPprintOrigProblem(scipSolver.get(), NULL, "cip", FALSE) );

   SCIPinfoMessage(scipSolver.get(), NULL, "\n");
   SCIP_CALL( SCIPpresolve(scipSolver.get()) );

   SCIPinfoMessage(scipSolver.get(), NULL, "Reformulated problem:\n");
   SCIP_CALL( SCIPprintTransProblem(scipSolver.get(), NULL, "cip", FALSE) );

   SCIPinfoMessage(scipSolver.get(), NULL, "\nSolving...\n");
   SCIP_CALL( SCIPsolve(scipSolver.get()) );

   if( SCIPgetNSols(scipSolver.get()) > 0 )
   {
      SCIPinfoMessage(scipSolver.get(), NULL, "\nSolution:\n");
      SCIP_CALL( SCIPprintSol(scipSolver.get(), SCIPgetBestSol(scipSolver.get()), NULL, FALSE) );
   }

   return SCIP_OKAY;
}


/** main method starting SCIP */
int main(
   int                   argc,               /**< number of arguments from the shell */
   char**                argv                /**< array of shell arguments */
   )
{  /*lint --e{715}*/
   SCIP_RETCODE retcode;

   retcode = runSpring();

   /* evaluate return code of the SCIP process */
   if( retcode != SCIP_OKAY )
   {
      /* write error back trace */
      SCIPprintError(retcode);
      return -1;
   }

   return 0;
}

/**
  Expected output

************************************************
* Running Coil Compression Spring Design Model *
************************************************

Original problem:
STATISTICS
  Problem name     : spring
  Variables        : 18 (11 binary, 1 integer, 0 implicit integer, 6 continuous)
  Constraints      : 0 initial, 9 maximal
OBJECTIVE
  Sense            : minimize
VARIABLES
  [binary] <wire1>: obj=0, original bounds=[0,1]
  [binary] <wire2>: obj=0, original bounds=[0,1]
  [binary] <wire3>: obj=0, original bounds=[0,1]
  [binary] <wire4>: obj=0, original bounds=[0,1]
  [binary] <wire5>: obj=0, original bounds=[0,1]
  [binary] <wire6>: obj=0, original bounds=[0,1]
  [binary] <wire7>: obj=0, original bounds=[0,1]
  [binary] <wire8>: obj=0, original bounds=[0,1]
  [binary] <wire9>: obj=0, original bounds=[0,1]
  [binary] <wire10>: obj=0, original bounds=[0,1]
  [binary] <wire11>: obj=0, original bounds=[0,1]
  [integer] <ncoils>: obj=0, original bounds=[0,+inf]
  [continuous] <volume>: obj=1, original bounds=[0,+inf]
  [continuous] <wirediam>: obj=0, original bounds=[0,+inf]
  [continuous] <deflection>: obj=0, original bounds=[0.00178571428571429,0.02]
  [continuous] <coildiam>: obj=0, original bounds=[0,+inf]
  [continuous] <const1>: obj=0, original bounds=[0,+inf]
  [continuous] <const2>: obj=0, original bounds=[0,+inf]
CONSTRAINTS
  [nonlinear] <voldef>: 1.5708*(2+<ncoils>)*<coildiam>*(<wirediam>)^2-<volume> == 0;
  [nonlinear] <defconst1>: <coildiam>*(<wirediam>)^(-1)-<const1> == 0;
  [nonlinear] <defconst2>: (-1+4*<const1>)*((-4+4*<const1>))^(-1)+0.615*(<const1>)^(-1)-<const2> == 0;
  [nonlinear] <shear>: 2546.48*<const1>*<const2>-189000*(<wirediam>)^2 <= 0;
  [nonlinear] <defdefl>: 6.95652e-07*<ncoils>*(<const1>)^3*(<wirediam>)^(-1)-<deflection> == 0;
  [nonlinear] <freel>: 1.05*<ncoils>*<wirediam>+1000*<deflection>+2.1*<wirediam> <= 14;
  [linear] <coilwidth>: <coildiam>[C] +<wirediam>[C] <= 3;
  [linear] <defwire>:  +0.207<wire1>[B] +0.225<wire2>[B] +0.244<wire3>[B] +0.263<wire4>[B] +0.283<wire5>[B] +0.307<wire6>[B] +0.331<wire7>[B] +0.362<wire8>[B] +0.394<wire9>[B] +0.4375<wire10>[B] +0.5<wire11>[B] -<wirediam>[C] == 0;
  [setppc] <selectwire>:  +<wire1>[B] +<wire2>[B] +<wire3>[B] +<wire4>[B] +<wire5>[B] +<wire6>[B] +<wire7>[B] +<wire8>[B] +<wire9>[B] +<wire10>[B] +<wire11>[B] == 1;
END

Changing lower bound for child of pow(.,-1) from 0 to 1e-09.
Check your model formulation or use option expr/pow/minzerodistance to avoid this warning.
Expression: (<t_wirediam>)^(-1)
presolving:
(round 1, fast)       0 del vars, 0 del conss, 0 add conss, 4 chg bounds, 0 chg sides, 0 chg coeffs, 0 upgd conss, 0 impls, 0 clqs
   (0.0s) probing cycle finished: starting next cycle
(round 2, exhaustive) 1 del vars, 0 del conss, 0 add conss, 5 chg bounds, 0 chg sides, 0 chg coeffs, 0 upgd conss, 90 impls, 39 clqs
   (0.0s) probing cycle finished: starting next cycle
   (0.0s) symmetry computation started: requiring (bin +, int -, cont -), (fixed: bin -, int +, cont +)
   (0.0s) no symmetry present
presolving (3 rounds: 3 fast, 2 medium, 2 exhaustive):
 1 deleted vars, 0 deleted constraints, 0 added constraints, 5 tightened bounds, 0 added holes, 0 changed sides, 0 changed coefficients
 90 implications, 44 cliques
presolved problem has 17 variables (10 bin, 1 int, 0 impl, 6 cont) and 9 constraints
      1 constraints of type <setppc>
      2 constraints of type <linear>
      6 constraints of type <nonlinear>
Presolving Time: 0.00

Solving...

 time | node  | left  |LP iter|LP it/n|mem/heur|mdpt |vars |cons |rows |cuts |sepa|confs|strbr|  dualbound   | primalbound  |  gap   | compl.
  0.0s|     1 |     0 |     4 |     - |   873k |   0 |  36 |   9 |  20 |   0 |  0 |   0 |   0 | 0.000000e+00 |      --      |    Inf | unknown
  0.0s|     1 |     0 |     7 |     - |   886k |   0 |  36 |   9 |  22 |   2 |  1 |   0 |   0 | 0.000000e+00 |      --      |    Inf | unknown
  0.0s|     1 |     0 |    13 |     - |   887k |   0 |  36 |   9 |  24 |   4 |  2 |   0 |   0 | 0.000000e+00 |      --      |    Inf | unknown
  0.0s|     1 |     0 |    15 |     - |   887k |   0 |  36 |   9 |  27 |   7 |  3 |   0 |   0 | 0.000000e+00 |      --      |    Inf | unknown
  0.0s|     1 |     0 |    19 |     - |   887k |   0 |  36 |   9 |  31 |  11 |  4 |   0 |   0 | 0.000000e+00 |      --      |    Inf | unknown
  0.0s|     1 |     0 |    29 |     - |   898k |   0 |  36 |   9 |  34 |  14 |  5 |   0 |   0 | 0.000000e+00 |      --      |    Inf | unknown
  0.0s|     1 |     0 |    33 |     - |   914k |   0 |  36 |   9 |  35 |  15 |  6 |   0 |   0 | 0.000000e+00 |      --      |    Inf | unknown
  0.0s|     1 |     0 |    35 |     - |   914k |   0 |  36 |   9 |  37 |  17 |  7 |   0 |   0 | 0.000000e+00 |      --      |    Inf | unknown
  0.0s|     1 |     0 |    37 |     - |   914k |   0 |  36 |   9 |  40 |  20 |  8 |   0 |   0 | 0.000000e+00 |      --      |    Inf | unknown
  0.0s|     1 |     0 |    38 |     - |   914k |   0 |  36 |   9 |  42 |  22 |  9 |   0 |   0 | 0.000000e+00 |      --      |    Inf | unknown
  0.0s|     1 |     0 |    39 |     - |   914k |   0 |  36 |   9 |  44 |  24 | 10 |   0 |   0 | 0.000000e+00 |      --      |    Inf | unknown
  0.0s|     1 |     0 |    40 |     - |   914k |   0 |  36 |   9 |  46 |  26 | 11 |   0 |   0 | 0.000000e+00 |      --      |    Inf | unknown
  0.0s|     1 |     0 |    41 |     - |   914k |   0 |  36 |   9 |  48 |  28 | 12 |   0 |   0 | 0.000000e+00 |      --      |    Inf | unknown
  0.0s|     1 |     0 |    42 |     - |   914k |   0 |  36 |   9 |  32 |  29 | 13 |   0 |   0 | 0.000000e+00 |      --      |    Inf | unknown
  0.0s|     1 |     0 |    43 |     - |   914k |   0 |  36 |   9 |  33 |  30 | 14 |   0 |   0 | 0.000000e+00 |      --      |    Inf | unknown
 time | node  | left  |LP iter|LP it/n|mem/heur|mdpt |vars |cons |rows |cuts |sepa|confs|strbr|  dualbound   | primalbound  |  gap   | compl.
  0.0s|     1 |     0 |   103 |     - |   916k |   0 |  36 |   9 |  25 |  30 | 16 |   0 |   0 | 1.221586e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   105 |     - |   916k |   0 |  36 |   9 |  27 |  32 | 17 |   0 |   0 | 1.221586e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   107 |     - |   917k |   0 |  36 |   9 |  29 |  34 | 18 |   0 |   0 | 1.221586e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   112 |     - |   918k |   0 |  36 |   9 |  31 |  36 | 19 |   0 |   0 | 1.221586e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   115 |     - |   918k |   0 |  36 |   9 |  33 |  38 | 20 |   0 |   0 | 1.221586e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   127 |     - |   925k |   0 |  36 |   9 |  26 |  40 | 21 |   0 |   0 | 1.221586e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   133 |     - |   925k |   0 |  36 |   9 |  29 |  43 | 22 |   0 |   0 | 1.500160e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   133 |     - |   925k |   0 |  36 |   9 |  29 |  43 | 22 |   0 |   0 | 1.500160e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   140 |     - |   929k |   0 |  36 |   9 |  33 |  47 | 23 |   0 |   0 | 1.778301e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   140 |     - |   929k |   0 |  36 |   9 |  33 |  47 | 23 |   0 |   0 | 1.778301e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   142 |     - |   946k |   0 |  36 |   9 |  35 |  49 | 24 |   0 |   0 | 1.789111e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   145 |     - |   954k |   0 |  36 |   9 |  38 |  52 | 25 |   0 |   0 | 1.793719e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   147 |     - |   958k |   0 |  36 |   9 |  34 |  54 | 26 |   0 |   0 | 1.794806e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   148 |     - |   961k |   0 |  36 |   9 |  36 |  56 | 27 |   0 |   0 | 1.795078e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   151 |     - |   965k |   0 |  36 |   9 |  39 |  59 | 28 |   0 |   0 | 1.795151e-01 |      --      |    Inf | unknown
 time | node  | left  |LP iter|LP it/n|mem/heur|mdpt |vars |cons |rows |cuts |sepa|confs|strbr|  dualbound   | primalbound  |  gap   | compl.
  0.0s|     1 |     0 |   155 |     - |   981k |   0 |  36 |   9 |  42 |  62 | 29 |   0 |   0 | 1.795166e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   164 |     - |   995k |   0 |  36 |   9 |  43 |  65 | 30 |   0 |   0 | 1.819380e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   165 |     - |   999k |   0 |  36 |   9 |  45 |  67 | 31 |   0 |   0 | 1.827110e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   211 |     - |  1013k |   0 |  36 |   9 |  43 |  67 | 33 |   0 |   3 | 1.832825e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   214 |     - |  1013k |   0 |  36 |   9 |  46 |  70 | 34 |   0 |   3 | 2.015227e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   214 |     - |  1013k |   0 |  36 |   9 |  45 |  70 | 34 |   0 |   3 | 2.015227e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   214 |     - |  1013k |   0 |  36 |   9 |  45 |  70 | 35 |   0 |   3 | 2.015227e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   222 |     - |  1013k |   0 |  36 |   9 |  46 |  71 | 36 |   0 |   3 | 2.246453e-01 |      --      |    Inf | unknown
  0.0s|     1 |     0 |   222 |     - |  1014k |   0 |  36 |   9 |  46 |  71 | 36 |   0 |   3 | 2.246453e-01 |      --      |    Inf | unknown
  0.0s|     1 |     2 |   222 |     - |  1014k |   0 |  36 |   9 |  46 |  71 | 37 |   0 |   3 | 2.246453e-01 |      --      |    Inf | unknown
* 0.0s|     7 |     2 |   339 |  36.7 |    LP  |   4 |  36 |   9 |  38 | 135 |  2 |   0 |  19 | 2.710053e-01 | 2.520874e+00 | 830.19%|  56.25%
* 0.1s|    47 |    18 |   892 |  16.8 |    LP  |  13 |  36 |   9 |  41 | 403 |  5 |   0 |  64 | 6.878695e-01 | 1.870464e+00 | 171.92%|  35.41%
* 0.1s|    52 |    15 |   940 |  16.1 |    LP  |  14 |  36 |   9 |  57 | 430 |  6 |   0 |  66 | 7.472538e-01 | 1.848249e+00 | 147.34%|  50.12%
* 0.1s|    65 |    12 |  1054 |  14.6 |    LP  |  14 |  36 |   9 |  49 | 502 |  2 |   0 |  68 | 7.472538e-01 | 1.718551e+00 | 129.98%|  64.84%
* 0.1s|    73 |     0 |  1064 |  13.1 |    LP  |  14 |  36 |   9 |  53 | 504 |  2 |   0 |  68 | 1.692491e+00 | 1.692491e+00 |   0.00%|  98.53%

SCIP Status        : problem is solved [optimal solution found]
Solving Time (sec) : 0.08
Solving Nodes      : 73
Primal Bound       : +1.69249101344105e+00 (5 solutions)
Dual Bound         : +1.69249101344105e+00
Gap                : 0.00 %

Solution:
objective value:                     1.69249101344105
wire5                                               1 	(obj:0)
ncoils                                              9 	(obj:0)
volume                               1.69249101344105 	(obj:1)
wirediam                                        0.283 	(obj:0)
deflection                        0.00178571328571429 	(obj:0)
coildiam                             1.22304078043973 	(obj:0)
const1                               4.32169887081178 	(obj:0)
const2                               1.36809322547192 	(obj:0)

  **/

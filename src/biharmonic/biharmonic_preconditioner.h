//LIC// ====================================================================
//LIC// This file forms part of oomph-lib, the object-oriented, 
//LIC// multi-physics finite-element library, available 
//LIC// at http://www.oomph-lib.org.
//LIC// 
//LIC// Copyright (C) 2006-2021 Matthias Heil and Andrew Hazel
//LIC// 
//LIC// This library is free software; you can redistribute it and/or
//LIC// modify it under the terms of the GNU Lesser General Public
//LIC// License as published by the Free Software Foundation; either
//LIC// version 2.1 of the License, or (at your option) any later version.
//LIC// 
//LIC// This library is distributed in the hope that it will be useful,
//LIC// but WITHOUT ANY WARRANTY; without even the implied warranty of
//LIC// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//LIC// Lesser General Public License for more details.
//LIC// 
//LIC// You should have received a copy of the GNU Lesser General Public
//LIC// License along with this library; if not, write to the Free Software
//LIC// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
//LIC// 02110-1301  USA.
//LIC// 
//LIC// The authors may be contacted at oomph-lib@maths.man.ac.uk.
//LIC// 
//LIC//====================================================================
//Include guards
#ifndef OOMPH_BIHARMONIC_PRECONDITIONER_HEADER
#define OOMPH_BIHARMONIC_PRECONDITIONER_HEADER


// Config header generated by autoconfig
#ifdef HAVE_CONFIG_H
  #include <oomph-lib-config.h>
#endif

#include "../generic/preconditioner.h"
#include "../generic/block_preconditioner.h"
#include "../generic/hijacked_elements.h"
#include "biharmonic_elements.h"
#include "../meshes/hermite_element_quad_mesh.template.h"
#include "../generic/SuperLU_preconditioner.h"
#include "../generic/general_purpose_preconditioners.h"

#ifdef OOMPH_HAS_HYPRE
#include "../generic/hypre_solver.h"
#endif

namespace oomph
{


#ifdef OOMPH_HAS_HYPRE

//=============================================================================
// defaults settings for the Hypre solver (AMG) when used as the approximate
// linear solver for the Schur complement (non-compound) linear subsidiary 
// linear systems
//=============================================================================
namespace Biharmonic_schur_complement_Hypre_defaults
{

 /// smoother type - Gauss Seidel: 1
 extern unsigned AMG_smoother;

 /// amg coarsening strategy: classical Ruge Stueben: 1
 extern unsigned AMG_coarsening;

 /// number of V cycles: 2
 extern unsigned N_cycle;

 /// amg strength parameter: 0.25 -- optimal for 2d
 extern double AMG_strength;

 /// jacobi damping -- hierher not used 0.1;
 extern double AMG_jacobi_damping;

 /// Amg smoother iterations: 2
 extern unsigned AMG_smoother_iterations;

 /// set the defaults
 extern void set_defaults(HyprePreconditioner* hypre_prec_pt);

}
#endif



//=============================================================================
/// \short Biharmonic Preconditioner - for two dimensional problems
//=============================================================================
 class BiharmonicPreconditioner : public BlockPreconditioner<CRDoubleMatrix>
  {
   
   public :
    
   /// Constructor - by default inexact preconditioning is used
    BiharmonicPreconditioner()
    {
     // initialise both preconditioners to zero
     Sub_preconditioner_1_pt = 0;
     Sub_preconditioner_2_pt = 0;
     Hijacked_sub_block_preconditioner_pt = 0;
     
     // by default we use the inexact biharmonic preconditioner
     // and if possible use Hypre preconditioner
#ifdef OOMPH_HAS_HYPRE
     Preconditioner_type = 2;
#else
     Preconditioner_type = 1;
#endif
     
     // size mesh pt correctly
     this->set_nmesh(1);
     Bulk_element_mesh_pt = 0;
    }
   
   /// destructor - cleans up preconditioners and delete matrices
   ~BiharmonicPreconditioner()
    {
     this->clean_up_memory();
    }

   // delete the subsidiary preconditioners and memory
   void clean_up_memory()
    {
     // delete the sub preconditioners
     delete Sub_preconditioner_1_pt;
     delete Sub_preconditioner_2_pt;
     delete Hijacked_sub_block_preconditioner_pt;
    }
   
   /// Broken copy constructor
   BiharmonicPreconditioner(const BiharmonicPreconditioner&) 
    { 
     BrokenCopy::broken_copy("BiharmonicPreconditioner");
    } 
   
   /// Broken assignment operator
   void operator=(const BiharmonicPreconditioner&) 
    {
     BrokenCopy::broken_assign("BiharmonicPreconditioner");
    }
   
   /// \short Setup the preconditioner 
   void setup();
   
   /// Apply preconditioner to r
   void preconditioner_solve(const DoubleVector &r, DoubleVector &z);
   
   /// \short Access function to the preconditioner type \n
   /// + 0 : exact BBD \n
   /// + 1 : inexact BBD w/ SuperLU \n
   /// + 2 : inexact BBD w/ AMG (Hypre Boomer AMG)
   /// + 3 : block diagonal (3x3)+(1x1)
   unsigned& preconditioner_type()
    {
     return Preconditioner_type;
    }
   
   /// \short Access function to the mesh pt for the bulk elements. The mesh
   /// should only contain BiharmonicElement<2> and 
   /// Hijacked<BiharmonicElement<2> > elements
   Mesh*& bulk_element_mesh_pt()
    {
     return Bulk_element_mesh_pt;
    }

    private:

   /// preconditioner type \n
   /// + 0 : exact BBD \n
   /// + 1 : inexact BBD w/ SuperLU \n
   /// + 2 : inexact BBD w/ AMG
   /// + 3 : block diagonal (3x3)+(1x1)
   unsigned Preconditioner_type;

   /// Exact Preconditioner Pointer
   Preconditioner* Sub_preconditioner_1_pt;
   
   /// Inexact Preconditioner Pointer
   Preconditioner* Sub_preconditioner_2_pt;
   
   /// Preconditioner the diagonal block associated with hijacked elements
   Preconditioner* Hijacked_sub_block_preconditioner_pt;

   /// the bulk element mesh pt
   Mesh* Bulk_element_mesh_pt;
  };
 
 
 
//=============================================================================
/// \short Sub Biharmonic Preconditioner - an exact preconditioner for the 
/// 3x3 top left hand corner sub block matrix.
/// Used as part of the BiharmonicPreconditioner<MATRIX> .
/// By default this uses the BBD (block-bordered-diagonal/arrow-shaped)
/// preconditioner; can also switch to full BD version (in which case
/// all the 3x3 blocks are retained)
//=============================================================================
class ExactSubBiharmonicPreconditioner 
 : public BlockPreconditioner<CRDoubleMatrix>
 {
  
  public :
   
   /// \short Constructor - for a preconditioner acting as a sub preconditioner
  ExactSubBiharmonicPreconditioner(BiharmonicPreconditioner* master_prec_pt,
                                   const bool& retain_all_blocks=false) :
  Retain_all_blocks(retain_all_blocks)
   {
    // Block mapping for ExactSubBiharmonicPreconditioner
    Vector<unsigned> block_lookup(3);
    block_lookup[0] = 0;
    block_lookup[1] = 1;
    block_lookup[2] = 2;
    
    // set as subsidiary block preconditioner
    this->turn_into_subsidiary_block_preconditioner(master_prec_pt,
                                                    block_lookup);

    // null the Sub preconditioner pt
    Sub_preconditioner_pt = 0;
   }
  
  /// destructor deletes the exact preconditioner
  ~ExactSubBiharmonicPreconditioner()
   {
    this->clean_up_memory();
   }

  /// delete the subsidiary preconditioner pointer
  void clean_up_memory()
   {
    delete Sub_preconditioner_pt; Sub_preconditioner_pt = 0;
   }
  
  /// Broken copy constructor
  ExactSubBiharmonicPreconditioner(const ExactSubBiharmonicPreconditioner&) 
   { 
    BrokenCopy::broken_copy("ExactSubBiharmonicPreconditioner");
   } 
  
  /// Broken assignment operator
  void operator=(const ExactSubBiharmonicPreconditioner&) 
   {
    BrokenCopy::broken_assign("ExactSubBiharmonicPreconditioner");
   }
  
  /// \short Setup the preconditioner 
  void setup();
  
  /// Apply preconditioner to r
  void preconditioner_solve(const DoubleVector &r, DoubleVector &z);
  
//   private:

  // Pointer to the sub preconditioner
  Preconditioner* Sub_preconditioner_pt;


  /// Boolean indicating that all blocks are to be retained (defaults to false)
  bool Retain_all_blocks;

 };
 
//=============================================================================
/// \short SubBiharmonic Preconditioner  - an inexact preconditioner for the 
/// 3x3 top left hand corner sub block matrix.
/// Used as part of the BiharmonicPreconditioner<MATRIX> 
//=============================================================================
 class InexactSubBiharmonicPreconditioner 
  : public BlockPreconditioner<CRDoubleMatrix>
  {
   
   public :
    
    /// \short Constructor for the inexact block preconditioner. \n
    /// This a helper class for BiharmonicPreconditioner and cannot be used
    /// as a standalone preconditioner. \n
    /// master_prec_pt is the pointer to the master BiharmonicPreconditioner.
    InexactSubBiharmonicPreconditioner
    (BiharmonicPreconditioner* master_prec_pt, const bool use_amg)
    {
     // Block mapping for ExactSubBiharmonicPreconditioner
     Vector<unsigned> block_lookup(3);
     block_lookup[0] = 0;
     block_lookup[1] = 1;
     block_lookup[2] = 2;
     
     // set as subsidiary block preconditioner
     this->turn_into_subsidiary_block_preconditioner
      (master_prec_pt,block_lookup);
     
     // zero all the preconditioner pt
     S_00_preconditioner_pt = 0;
     Lumped_J_11_preconditioner_pt = 0;
     Lumped_J_22_preconditioner_pt = 0;

     // store the preconditioner type
     Use_amg = use_amg;
    }
   
   /// destructor  - just calls this->clean_up_memory()
   ~InexactSubBiharmonicPreconditioner()
    {
     this->clean_up_memory();
    }
     
   /// clean the memory
   void clean_up_memory()
    {
     // delete the sub preconditioner pt
     delete S_00_preconditioner_pt;
     S_00_preconditioner_pt = 0;
     
     // delete the lumped preconditioners
     delete Lumped_J_11_preconditioner_pt;
     Lumped_J_11_preconditioner_pt = 0;
     delete Lumped_J_22_preconditioner_pt;
     Lumped_J_22_preconditioner_pt = 0;

     // Number of block types
     unsigned n = Matrix_of_block_pointers.nrow();

     // delete the block matrices
     for (unsigned i = 0; i < n; i++)
      {
       for (unsigned j = 0; j < n; j++)
        {
         if (Matrix_of_block_pointers(i,j) != 0)
          {
           delete Matrix_of_block_pointers(i,j);
           Matrix_of_block_pointers(i,j)=0;
          }
        }
      }
    }
   
   /// Broken copy constructor
   InexactSubBiharmonicPreconditioner
    (const InexactSubBiharmonicPreconditioner&) 
    { 
     BrokenCopy::broken_copy("InexactSubBiharmonicPreconditioner");
    } 
   
   /// Broken assignment operator
   void operator=(const InexactSubBiharmonicPreconditioner&) 
    {
     BrokenCopy::broken_assign("InexactSubBiharmonicPreconditioner");
    }

   /// \short Setup the preconditioner 
   void setup();
   
   /// Apply preconditioner to r
   void preconditioner_solve(const DoubleVector &r, DoubleVector &z);
   
//    private:

   /// \short Computes the inexact schur complement of the block J_00 using
   /// lumping as an approximate inverse of block J_11 and J_22 
   /// (where possible)
   void compute_inexact_schur_complement();
   
   /// \short Pointer to the S00 Schur Complement preconditioner
   Preconditioner* S_00_preconditioner_pt;
   
   /// \short Preconditioner for storing the lumped J_11 matrix
   MatrixBasedLumpedPreconditioner<CRDoubleMatrix>* 
    Lumped_J_11_preconditioner_pt;
   
   /// \short Preconditioner for storing the lumped J_22 matrix
   MatrixBasedLumpedPreconditioner<CRDoubleMatrix>* 
    Lumped_J_22_preconditioner_pt;

   ///
   DenseMatrix<CRDoubleMatrix*> Matrix_of_block_pointers;

   ///
   CRDoubleMatrix* S_00_pt;

   /// \short booean indicating whether (Hypre BoomerAMG) AMG should be used 
   /// to solve the S00 subsidiary linear system.
   unsigned Use_amg;
 };
}
#endif

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
//Header file for general linear elasticity elements

//Include guards to prevent multiple inclusion of the header
#ifndef OOMPH_LINEAR_ELASTICITY_ELEMENTS_HEADER
#define OOMPH_LINEAR_ELASTICITY_ELEMENTS_HEADER

// Config header generated by autoconfig
#ifdef HAVE_CONFIG_H
  #include <oomph-lib-config.h>
#endif


//OOMPH-LIB headers
#include "../generic/Qelements.h"
#include "../generic/mesh.h"
#include "../generic/hermite_elements.h"
#include "./elasticity_tensor.h"
#include "../generic/projection.h"

namespace oomph
{
//=======================================================================
/// A base class for elements that solve the equations of linear 
/// elasticity in Cartesian coordinates.
/// Combines a few generic functions that are shared by 
/// LinearElasticityEquations
/// and LinearElasticityEquationsWithPressure (hierher: The latter
/// don't exist yet but will be written as soon as somebody needs them...)
//=======================================================================
 template <unsigned DIM>
  class LinearElasticityEquationsBase : public virtual FiniteElement
  {
    public:
   
   /// \short Return the index at which the i-th unknown displacement 
   /// component is stored. The default value, i, is appropriate for
   /// single-physics problems.
   virtual inline unsigned u_index_linear_elasticity(const unsigned i) const
   {return i;}

   /// d^2u/dt^2 at local node n
   double d2u_dt2_linear_elasticity(const unsigned &n,
                                    const unsigned &i) const
    {
     // Get the timestepper
     TimeStepper* time_stepper_pt=node_pt(n)->time_stepper_pt();

     // Storage for the derivative - initialise to 0
     double d2u_dt2=0.0;

     // If we are doing an unsteady solve then calculate the derivative
     if(!time_stepper_pt->is_steady())
      {
       // Get the nodal index
       const unsigned u_nodal_index=u_index_linear_elasticity(i);

       // Get the number of values required to represent history
       const unsigned n_time=time_stepper_pt->ntstorage();

       // Loop over history values
       for(unsigned t=0;t<n_time;t++)
        {
         //Add the contribution to the derivative
         d2u_dt2+=time_stepper_pt->weight(2,t)*nodal_value(t,n,u_nodal_index);
        }
      }

     return d2u_dt2;
    }

   /// Compute vector of FE interpolated displacement u at local coordinate s
   void interpolated_u_linear_elasticity(const Vector<double> &s, 
                                         Vector<double>& disp) 
    const
    {
     //Find number of nodes
     unsigned n_node = nnode();

     //Local shape function
     Shape psi(n_node);
     
     //Find values of shape function
     shape(s,psi);
     
     for (unsigned i=0;i<DIM;i++)
      {
       //Index at which the nodal value is stored
       unsigned u_nodal_index = u_index_linear_elasticity(i);

       //Initialise value of u
       disp[i] = 0.0;

       //Loop over the local nodes and sum
       for(unsigned l=0;l<n_node;l++) 
        {
         disp[i] += nodal_value(l,u_nodal_index)*psi[l];
        }
      }
    }

   /// Return FE interpolated displacement u[i] at local coordinate s
   double interpolated_u_linear_elasticity(const Vector<double> &s, 
                                           const unsigned &i) const
    {
     //Find number of nodes
     unsigned n_node = nnode();

     //Local shape function
     Shape psi(n_node);

     //Find values of shape function
     shape(s,psi);
     
     //Get nodal index at which i-th velocity is stored
     unsigned u_nodal_index = u_index_linear_elasticity(i);
     
     //Initialise value of u
     double interpolated_u = 0.0;

     //Loop over the local nodes and sum
     for(unsigned l=0;l<n_node;l++) 
      {
       interpolated_u += nodal_value(l,u_nodal_index)*psi[l];
      }
     
     return(interpolated_u);
    }


   /// \short Function pointer to function that specifies the body force
   /// as a function of the Cartesian coordinates and time FCT(t,x,b) -- 
   /// x and b are  Vectors! 
   typedef void (*BodyForceFctPt)(const double& t,
                                  const Vector<double>& x,
                                  Vector<double>& b);
   
   /// \short Constructor: Set null pointers for constitutive law and for
   /// isotropic growth function. Set physical parameter values to 
   /// default values, switch on inertia and set body force to zero.
    LinearElasticityEquationsBase() : Elasticity_tensor_pt(0),
    Lambda_sq_pt(&Default_lambda_sq_value), Unsteady(true),
    Body_force_fct_pt(0) {}
   
   /// Return the pointer to the elasticity_tensor
   ElasticityTensor* &elasticity_tensor_pt() {return Elasticity_tensor_pt;}
   
   /// Access function to the entries in the elasticity tensor
   inline double E(const unsigned &i,const unsigned &j,
                   const unsigned &k, const unsigned &l) const
   {
    return (*Elasticity_tensor_pt)(i,j,k,l);
   }
   
   ///Access function for timescale ratio (nondim density)
   const double& lambda_sq() const {return *Lambda_sq_pt;}
   
   /// Access function for pointer to timescale ratio (nondim density)
   double* &lambda_sq_pt() {return Lambda_sq_pt;}
   
   /// Access function: Pointer to body force function
   BodyForceFctPt& body_force_fct_pt() {return Body_force_fct_pt;}
   
   /// Access function: Pointer to body force function (const version)
   BodyForceFctPt body_force_fct_pt() const {return Body_force_fct_pt;}
   

   /// Switch on solid inertia
   void enable_inertia() {Unsteady=true;}

   /// Switch off solid inertia
   void disable_inertia() {Unsteady=false;}
   
   ///Access function to flag that switches inertia on/off (const version)
   bool is_inertia_enabled() const {return Unsteady;}
      
   ///Pin the element's redundant solid pressures (needed for refinement)
   virtual void pin_elemental_redundant_nodal_solid_pressures() {}
   
   /// \short  Loop over all elements in Vector (which typically contains
   /// all the elements in a refineable solid mesh) and pin the nodal solid 
   /// pressure  degrees of freedom that are not being used. Function uses 
   /// the member function
   /// - \c LinearElasticityEquationsBase<DIM>::
   ///      pin_elemental_redundant_nodal_pressure_dofs()
   /// .
   /// which is empty by default and should be implemented for
   /// elements with nodal solid pressure degrees of freedom  
   /// (e.g. linear elasticity elements with continuous pressure interpolation.)
   static void pin_redundant_nodal_solid_pressures(
    const Vector<GeneralisedElement*>& element_pt)
   {
    // Loop over all elements and call the function that pins their
    // unused nodal solid pressure data
    unsigned n_element = element_pt.size();
    for(unsigned e=0;e<n_element;e++)
     {
      dynamic_cast<LinearElasticityEquationsBase<DIM>*>(element_pt[e])->
       pin_elemental_redundant_nodal_solid_pressures();
     }
   }
   
   /// \short Return the Cauchy stress tensor, as calculated
   /// from the elasticity tensor at specified local coordinate
   /// Virtual so separaete versions can (and must!) be provided
   /// for displacement and pressure-displacement formulations.
   virtual void get_stress(const Vector<double> &s, 
                           DenseMatrix<double> &sigma) const=0;
   
   /// \short Return the strain tensor
   void get_strain(const Vector<double> &s, DenseMatrix<double> &strain) const;
   
   /// \short Evaluate body force at Eulerian coordinate x at present time
   /// (returns zero vector if no body force function pointer has been set)
   inline void body_force(const Vector<double>& x, 
                          Vector<double>& b) const
   {
    //If no function has been set, return zero vector
    if(Body_force_fct_pt==0)
     {
      // Get spatial dimension of element
      unsigned n=dim();
      for (unsigned i=0;i<n;i++)
       {
        b[i] = 0.0;
       }
     }
    else
     {
      // Get time from timestepper of first node (note that this must
      // work -- body force only makes sense for elements that can be
      // deformed and given that the deformation of solid finite elements
      // is controlled by their nodes, nodes must exist!)
      double time=node_pt(0)->time_stepper_pt()->time_pt()->time();
      
      // Now evaluate the body force
      (*Body_force_fct_pt)(time,x,b);
     }
   }
   
   
   
   
   /// \short The number of "DOF types" that degrees of freedom in this element
   /// are sub-divided into: for now lump them all into one DOF type.
   /// Can be adjusted later
   unsigned ndof_types() const
   {
    return 2;
    //return 1;
   }
   
   /// \short Create a list of pairs for all unknowns in this element,
   /// so that the first entry in each pair contains the global equation
   /// number of the unknown, while the second one contains the number
   /// of the "DOF types" that this unknown is associated with.
   /// (Function can obviously only be called if the equation numbering
   /// scheme has been set up.) 
   void get_dof_numbers_for_unknowns(
    std::list<std::pair<unsigned long,unsigned> >& dof_lookup_list) const
   {

    // temporary pair (used to store dof lookup prior to being added 
    // to list)
    std::pair<unsigned long,unsigned> dof_lookup;
    
    // number of nodes
    const unsigned n_node = this->nnode();
    
    //Integer storage for local unknown
    int local_unknown=0;
    
    //Loop over the nodes
    for(unsigned n=0;n<n_node;n++)
     {
      //Loop over dimension
      for(unsigned i=0;i<DIM;i++)
       {
        //If the variable is free
        local_unknown = nodal_local_eqn(n,i);
        
        // ignore pinned values
        if (local_unknown >= 0)
         {
          // store dof lookup in temporary pair: First entry in pair
          // is global equation number; second entry is dof type
          dof_lookup.first = this->eqn_number(local_unknown);
          dof_lookup.second = i;
          //dof_lookup.second = DIM;
          
          // add to list
          dof_lookup_list.push_front(dof_lookup);
          
         }
       }
     }
   }
   
   
    protected:
   
   /// Pointer to the elasticity tensor
   ElasticityTensor *Elasticity_tensor_pt;
   
   /// Timescale ratio (non-dim. density)
   double* Lambda_sq_pt;
   
   /// Flag that switches inertia on/off
   bool Unsteady;
   
   /// Pointer to body force function
   BodyForceFctPt Body_force_fct_pt;
   
   /// Static default value for timescale ratio (1.0 -- for natural scaling) 
   static double Default_lambda_sq_value;
   
  };
 
 
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


//=======================================================================
/// A class for elements that solve the equations of linear elasticity
/// in cartesian coordinates.
//=======================================================================
 template <unsigned DIM>
  class LinearElasticityEquations : public virtual LinearElasticityEquationsBase<DIM>
  {
    public:
   
   /// \short  Constructor
   LinearElasticityEquations() {}
   
   /// Number of values required at node n.
   unsigned required_nvalue(const unsigned &n) const {return DIM;}
   
   /// \short Return the residuals for the solid equations (the discretised
   /// principle of virtual displacements)
   void fill_in_contribution_to_residuals(Vector<double> &residuals)
   {
    fill_in_generic_contribution_to_residuals_linear_elasticity(
     residuals,GeneralisedElement::Dummy_matrix,0);
   }
   
   /// The jacobian is calculated by finite differences by default,
   /// We need only to take finite differences w.r.t. positional variables
   /// For this element
   void fill_in_contribution_to_jacobian(Vector<double> &residuals,
                                         DenseMatrix<double> &jacobian)
   {
    //Add the contribution to the residuals
    this->fill_in_generic_contribution_to_residuals_linear_elasticity(
     residuals,jacobian,1);
   }
   
   /// \short Return the Cauchy stress tensor, as calculated
   /// from the elasticity tensor at specified local coordinate
   void get_stress(const Vector<double> &s, 
                   DenseMatrix<double> &sigma) const;


   ///Output exact solution x,y,[z],u,v,[w]
   void output_fct(std::ostream &outfile, 
                   const unsigned &nplot, 
                   FiniteElement::SteadyExactSolutionFctPt exact_soln_pt);

   ///Output exact solution x,y,[z],u,v,[w] (unsteady version)
   void output_fct(std::ostream &outfile, 
                   const unsigned &nplot, 
                   const double &time,
                   FiniteElement::UnsteadyExactSolutionFctPt exact_soln_pt);
   
   /// Output: x,y,[z],u,v,[w]
   void output(std::ostream &outfile) 
   {
    unsigned n_plot=5;
    output(outfile,n_plot);
   }
   
   /// Output: x,y,[z],u,v,[w]
   void output(std::ostream &outfile, const unsigned &n_plot);
   
   
   /// C-style output: x,y,[z],u,v,[w]
   void output(FILE* file_pt) 
   {
    unsigned n_plot=5;
    output(file_pt,n_plot);
   }
   
   /// Output: x,y,[z],u,v,[w]
   void output(FILE* file_pt, const unsigned &n_plot);
   
   /// \short Validate against exact solution.
   /// Solution is provided via function pointer.
   /// Plot at a given number of plot points and compute L2 error
   /// and L2 norm of displacement solution over element
   void compute_error(std::ostream &outfile,
                      FiniteElement::SteadyExactSolutionFctPt exact_soln_pt,
                      double& error, double& norm);

   /// \short Validate against exact solution.
   /// Solution is provided via function pointer.
   /// Plot at a given number of plot points and compute L2 error
   /// and L2 norm of displacement solution over element
   /// (unsteady version)
   void compute_error(std::ostream &outfile,
                      FiniteElement::UnsteadyExactSolutionFctPt exact_soln_pt,
                      const double& time, double& error, double& norm);
   
    private:


   /// \short Private helper function to compute residuals and (if requested
   /// via flag) also the Jacobian matrix.
   virtual void fill_in_generic_contribution_to_residuals_linear_elasticity(
    Vector<double> &residuals,DenseMatrix<double> &jacobian,unsigned flag);
      
  }; 


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////


//===========================================================================
/// An Element that solves the equations of linear elasticity 
/// in Cartesian coordinates, using QElements for the geometry
//============================================================================
 template<unsigned DIM, unsigned NNODE_1D>
  class QLinearElasticityElement : public virtual QElement<DIM,NNODE_1D>,
  public virtual LinearElasticityEquations<DIM>
  {
    public:
   
   /// Constructor
    QLinearElasticityElement() : QElement<DIM,NNODE_1D>(), 
    LinearElasticityEquations<DIM>() { }
   
   ///Output exact solution x,y,[z],u,v,[w]
   void output_fct(std::ostream &outfile, 
                   const unsigned &nplot, 
                   FiniteElement::SteadyExactSolutionFctPt exact_soln_pt)
   {
    LinearElasticityEquations<DIM>::output_fct(outfile,nplot,exact_soln_pt);
   }
   
   /// Output function
   void output(std::ostream &outfile) 
   {LinearElasticityEquations<DIM>::output(outfile);}
   
   /// Output function
   void output(std::ostream &outfile, const unsigned &n_plot)
   {LinearElasticityEquations<DIM>::output(outfile,n_plot);}
   
   
   /// C-style output function
   void output(FILE* file_pt) 
   {LinearElasticityEquations<DIM>::output(file_pt);}
   
   /// C-style output function
   void output(FILE* file_pt, const unsigned &n_plot)
   {LinearElasticityEquations<DIM>::output(file_pt,n_plot);}
   
  };
 

//============================================================================
/// FaceGeometry of a linear 2D QLinearElasticityElement element
//============================================================================
 template<>
  class FaceGeometry<QLinearElasticityElement<2,2> > :
 public virtual QElement<1,2>
  {
    public:
   /// Constructor must call the constructor of the underlying solid element
    FaceGeometry() : QElement<1,2>() {}
  };
 
 
 
//============================================================================
/// FaceGeometry of a quadratic 2D QLinearElasticityElement element
//============================================================================
 template<>
  class FaceGeometry<QLinearElasticityElement<2,3> > :
 public virtual QElement<1,3>
  {
    public:
   /// Constructor must call the constructor of the underlying element
    FaceGeometry() : QElement<1,3>() {}
  };
 
 
 
//============================================================================
/// FaceGeometry of a cubic 2D QLinearElasticityElement element
//============================================================================
 template<>
  class FaceGeometry<QLinearElasticityElement<2,4> > :
  public virtual QElement<1,4>
  {
    public:
   /// Constructor must call the constructor of the underlying element
    FaceGeometry() : QElement<1,4>() {}
  };
  
  
//============================================================================
/// FaceGeometry of a linear 3D QLinearElasticityElement element
//============================================================================
  template<>
   class FaceGeometry<QLinearElasticityElement<3,2> > :
  public virtual QElement<2,2>
   {
     public:
    /// Constructor must call the constructor of the underlying element
     FaceGeometry() : QElement<2,2>() {}
   };
  
//============================================================================
/// FaceGeometry of a quadratic 3D QLinearElasticityElement element
//============================================================================
  template<>
   class FaceGeometry<QLinearElasticityElement<3,3> > :
  public virtual QElement<2,3>
   {
     public:
    /// Constructor must call the constructor of the underlying element
     FaceGeometry() : QElement<2,3>() {}
   };
  
  
//============================================================================
/// FaceGeometry of a cubic 3D QLinearElasticityElement element
//============================================================================
  template<>
   class FaceGeometry<QLinearElasticityElement<3,4> > :
  public virtual QElement<2,4>
   {
     public:
    /// Constructor must call the constructor of the underlying element
     FaceGeometry() : QElement<2,4>() {}
   };

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////


//==========================================================
/// Linear elasticity upgraded to become projectable
//==========================================================
 template<class LINEAR_ELAST_ELEMENT>
 class ProjectableLinearElasticityElement : 
  public virtual ProjectableElement<LINEAR_ELAST_ELEMENT>
 {
  
 public:
  
  /// \short Constructor [this was only required explicitly
  /// from gcc 4.5.2 onwards...]
  ProjectableLinearElasticityElement(){}
  
  
  /// \short Specify the values associated with field fld. 
  /// The information is returned in a vector of pairs which comprise 
  /// the Data object and the value within it, that correspond to field fld. 
  /// In the underlying linear elasticity elements the 
  /// the displacements are stored at the nodal values
  Vector<std::pair<Data*,unsigned> > data_values_of_field(const unsigned& fld)
   {   
    // Create the vector
    Vector<std::pair<Data*,unsigned> > data_values;
    
    // Loop over all nodes and extract the fld-th nodal value
    unsigned nnod=this->nnode();
    for (unsigned j=0;j<nnod;j++)
     {
      // Add the data value associated with the displacement components
      data_values.push_back(std::make_pair(this->node_pt(j),fld));
     }
    
    // Return the vector
    return data_values;
    
   }

  /// \short Number of fields to be projected: dim, corresponding to 
  /// the displacement components
  unsigned nfields_for_projection()
   {
    return this->dim();
   }
  
  /// \short Number of history values to be stored for fld-th field. 
  /// (includes present value!)
  unsigned nhistory_values_for_projection(const unsigned &fld)
   {
#ifdef PARANOID
    if (fld>1)
     {
      std::stringstream error_stream;
      error_stream 
       << "Elements only store two fields so fld can't be"
       << " " << fld << std::endl;
      throw OomphLibError(
       error_stream.str(),
       OOMPH_CURRENT_FUNCTION,
       OOMPH_EXCEPTION_LOCATION);
     }
#endif
   return this->node_pt(0)->ntstorage();   
   }
  
  ///\short Number of positional history values: Read out from
  /// positional timestepper  (Note: count includes current value!)
  unsigned nhistory_values_for_coordinate_projection()
   {
    return this->node_pt(0)->position_time_stepper_pt()->ntstorage();
   }
  
  /// \short Return Jacobian of mapping and shape functions of field fld
  /// at local coordinate s
  double jacobian_and_shape_of_field(const unsigned &fld, 
                                     const Vector<double> &s, 
                                     Shape &psi)
   {
    unsigned n_dim=this->dim();
    unsigned n_node=this->nnode();
    DShape dpsidx(n_node,n_dim);
        
    // Call the derivatives of the shape functions and return
    // the Jacobian
    return this->dshape_eulerian(s,psi,dpsidx);
   }
  


  /// \short Return interpolated field fld at local coordinate s, at time level
  /// t (t=0: present; t>0: history values)
  double get_field(const unsigned &t, 
                   const unsigned &fld,
                   const Vector<double>& s)
   {
    unsigned n_node=this->nnode();

/* #ifdef PARANOID */
/*     unsigned n_dim=this->node_pt(0)->ndim(); */
/* #endif */
    
    //Local shape function
    Shape psi(n_node);
    
    //Find values of shape function
    this->shape(s,psi);
    
    //Initialise value of u
    double interpolated_u = 0.0;
    
    //Sum over the local nodes at that time
    for(unsigned l=0;l<n_node;l++) 
     {
// over-zealous I think. This will quietly do the right thing
// even if there are additional degrees of freedom floating around.
/* #ifdef PARANOID */
/*       unsigned nvalue=this->node_pt(l)->nvalue(); */
/*       if (nvalue!=n_dim) */
/*        {         */
/*         std::stringstream error_stream; */
/*         error_stream  */
/*          << "Current implementation only works for non-resized nodes\n" */
/*          << "but nvalue= " << nvalue << "!= dim = " << n_dim << std::endl; */
/*         throw OomphLibError( */
/*          error_stream.str(), */
/*          OOMPH_CURRENT_FUNCTION, */
/*          OOMPH_EXCEPTION_LOCATION); */
/*        } */
/* #endif */
      interpolated_u += this->nodal_value(t,l,fld)*psi[l];
     }
    return interpolated_u;     
   }
  
 
  ///Return number of values in field fld
  unsigned nvalue_of_field(const unsigned &fld)
   {
    return this->nnode();
   }
  
  
  ///Return local equation number of value j in field fld.
  int local_equation(const unsigned &fld,
                     const unsigned &j)
   {
// over-zealous I think. This will quietly do the right thing
// even if there are additional degrees of freedom floating around.
/* #ifdef PARANOID */
/*     unsigned n_dim=this->node_pt(0)->ndim(); */
/*     unsigned nvalue=this->node_pt(j)->nvalue(); */
/*     if (nvalue!=n_dim) */
/*      {         */
/*       std::stringstream error_stream; */
/*       error_stream  */
/*        << "Current implementation only works for non-resized nodes\n" */
/*        << "but nvalue= " << nvalue << "!= dim = " << n_dim << std::endl; */
/*       throw OomphLibError( */
/*          error_stream.str(), */
/*          OOMPH_CURRENT_FUNCTION, */
/*          OOMPH_EXCEPTION_LOCATION); */
/*      } */
/* #endif */
    return this->nodal_local_eqn(j,fld);
   }

  
 };


//=======================================================================
/// Face geometry for element is the same as that for the underlying
/// wrapped element
//=======================================================================
 template<class ELEMENT>
 class FaceGeometry<ProjectableLinearElasticityElement<ELEMENT> > 
  : public virtual FaceGeometry<ELEMENT>
 {
 public:
  FaceGeometry() : FaceGeometry<ELEMENT>() {}
 };


//=======================================================================
/// Face geometry of the Face Geometry for element is the same as 
/// that for the underlying wrapped element
//=======================================================================
 template<class ELEMENT>
 class FaceGeometry<FaceGeometry<ProjectableLinearElasticityElement<ELEMENT> > >
  : public virtual FaceGeometry<FaceGeometry<ELEMENT> >
 {
 public:
  FaceGeometry() : FaceGeometry<FaceGeometry<ELEMENT> >() {}
 };


}

#endif




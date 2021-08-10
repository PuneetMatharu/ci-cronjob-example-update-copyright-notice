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
// Config header generated by autoconfig
#ifdef HAVE_CONFIG_H
  #include <oomph-lib-config.h>
#endif

//oomph-lib includes
#include "biharmonic_problem.h"


namespace oomph
{

//=============================================================================
/// \short Impose a Clamped Edge. Imposes the prescribed dirichlet BCs u and 
/// du/dn dirichlet BCs by pinning
//=============================================================================
template<unsigned DIM>
void BiharmonicProblem<DIM>::
set_dirichlet_boundary_condition(const unsigned& b, DirichletBCFctPt u_fn,
                                 DirichletBCFctPt dudn_fn)
{
 // number of nodes on boundary b
 unsigned n_node = Bulk_element_mesh_pt->nboundary_node(b);

 // fixed faced index for boundary
 int face_index = Bulk_element_mesh_pt->face_index_at_boundary(b,0);

 //Need to get the s_fixed_index
 unsigned s_fixed_index = 0;
 switch(face_index)
  {
  case -1:
  case 1:
   s_fixed_index = 0;
   break;

  case -2:
  case 2:
   s_fixed_index = 1;
   break;
   
  default:
   throw OomphLibError("Face Index not +/-1 or +/-2: Need 2D QElements",
                       OOMPH_CURRENT_FUNCTION,
                       OOMPH_EXCEPTION_LOCATION);
  } 
 
 // node position along edge b in macro element boundary representation 
 // [-1,1]
 Vector<double> s(1);
 
 //Set the edge sign
 int edge_sign = 0;
 switch(face_index)
  {
  case -1:
  case 2:
   edge_sign = -1;
   break;
   
  case 1:
  case -2:
   edge_sign = 1;
   break;
  }
 
 // finite difference step
 const double h = 10e-8;
 
 // node position along edge b in macro element boundary representation 
 // [-1,1]
 Vector<double> m(2);

 // if u is prescribed then impose
 if (u_fn != 0)
  {
   
   // loop over nodes on boundary b                                         
   for (unsigned n = 0; n < n_node; n++)              
    {                                        
     
     // find node position along edge [-1,1] in macro element representation
     Bulk_element_mesh_pt->boundary_node_pt(b,n)->
      get_coordinates_on_boundary(b,m);
     
     // get u at node
     double u;
     (*u_fn)(m[0],u);
     
     // finite difference is used to compute dudm_t
     // u0 and u1 store values of u left/below and right/above node n
     double u_L, u_R;
     
     // if left/lower corner node
     if (n == 0)
      {
       (*u_fn)(m[0],u_L);
       (*u_fn)(m[0]+h,u_R);
      }
     // if right/upper corner node
     else if (n == n_node-1)
      {
       (*u_fn)(m[0]-h,u_L);
       (*u_fn)(m[0],u_R);
      }
     // if other node
     else 
      {
       (*u_fn)(m[0]-0.5*h,u_L);
       (*u_fn)(m[0]+0.5*h,u_R);
      }
     
     // compute dudm_t
     double dudm_t = (u_R-u_L)/h;
     
     //compute duds_t
     double duds_t = m[1]*dudm_t;
     
     // pin and set u type dof
     Bulk_element_mesh_pt->boundary_node_pt(b,n)->pin(0);
     Bulk_element_mesh_pt->boundary_node_pt(b,n)->set_value(0,u);
     
     // pin and set duds_t type degree of freedom  
     Bulk_element_mesh_pt->boundary_node_pt(b,n)->pin(2-s_fixed_index);
     Bulk_element_mesh_pt->boundary_node_pt(b,n)
      ->set_value(2-s_fixed_index,duds_t);
    }                                                          
  }

 // if dudn is prescribed then impose
 if (dudn_fn != 0)
  {
   
   // loop over nodes on boundary b                                         
   for (unsigned n = 0; n < n_node; n++)              
    {
     // vectors for dx_i/ds_n and dx_i/ds_t
     Vector<double> dxds_n(2);
     Vector<double> dxds_t(2);
     dxds_n[0] = 
      Bulk_element_mesh_pt->boundary_node_pt(b,n)->x_gen(1+s_fixed_index,0);
     dxds_n[1] = 
      Bulk_element_mesh_pt->boundary_node_pt(b,n)->x_gen(1+s_fixed_index,1);
     dxds_t[0] = 
      Bulk_element_mesh_pt->boundary_node_pt(b,n)->x_gen(2-s_fixed_index,0);
     dxds_t[1] = 
      Bulk_element_mesh_pt->boundary_node_pt(b,n)->x_gen(2-s_fixed_index,1);
     
     // vector for d2xi/ds_n ds_t
     Vector<double> d2xds_nds_t(2);
     d2xds_nds_t[0] = Bulk_element_mesh_pt->boundary_node_pt(b,n)->x_gen(3,0);
     d2xds_nds_t[1] = Bulk_element_mesh_pt->boundary_node_pt(b,n)->x_gen(3,1);
     
     // compute dn/ds_n
     double dnds_n = ((dxds_n[0]*dxds_t[1])-(dxds_n[1]*dxds_t[0]))
      / (sqrt(dxds_t[0]*dxds_t[0]+dxds_t[1]*dxds_t[1])*edge_sign);
     
     // compute dt/ds_n
     double dtds_n = ((dxds_n[0]*dxds_t[0])+(dxds_n[1]*dxds_t[1]))
      /sqrt(dxds_t[0]*dxds_t[0]+dxds_t[1]*dxds_t[1]);
     
     // compute dt/ds_t
     double dtds_t = sqrt(pow(dxds_t[0],2)+pow(dxds_t[1],2));

     // compute ds_n/dn
     double ds_ndn = -1.0*(edge_sign*sqrt(pow(dxds_t[0],2)+pow(dxds_t[1],2)))/
      (dxds_t[0]*dxds_n[1]-dxds_n[0]*dxds_t[1]);

     // compute ds_t/dt
     double ds_tdt = 1/sqrt(dxds_t[0]*dxds_t[0]+dxds_t[1]*dxds_t[1]);

     // compute d2t/ds_nds_t
     double d2tds_nds_t = (dxds_t[0]*d2xds_nds_t[0] + dxds_t[1]*d2xds_nds_t[1])
      /sqrt(dxds_t[0]*dxds_t[0] + dxds_t[1]*dxds_t[1]);

     // compute d2s_t/ds_ndt
     double d2s_tds_ndt = (dxds_t[0]*d2xds_nds_t[0] + dxds_t[1]*d2xds_nds_t[1])
      /pow(dxds_t[0]*dxds_t[0] + dxds_t[1]*dxds_t[1],3.0/2.0);

     // get m_t and dm_t/ds_t for this node
     Vector<double> m_N(2);
     Bulk_element_mesh_pt->boundary_node_pt(b,n)
      ->get_coordinates_on_boundary(b,m_N);

     // compute d2u/dm_t2 and d/dm_t(dudn) by finite difference
     double d2udm_t2 = 0;
     double ddm_tdudn = 0;
     double u_2L,u_N,u_2R,dudn_L,dudn_R;
     // evaluate u_fn and dudn_fn for current node
     if (n == 0)
      {
       (*u_fn)(m_N[0],u_2L);
       (*u_fn)(m_N[0]+h,u_N);
       (*u_fn)(m_N[0]-2*h,u_2R);
       (*dudn_fn)(m_N[0],dudn_L);
       (*dudn_fn)(m_N[0]+h,dudn_R);
      }
     else if (n == n_node-1)
      {
       (*u_fn)(m_N[0]-2*h,u_2L);
       (*u_fn)(m_N[0]-h,u_N);
       (*u_fn)(m_N[0],u_2R);
       (*dudn_fn)(m_N[0]-h,dudn_L);
       (*dudn_fn)(m_N[0],dudn_R);
      }
     else
      {
       (*u_fn)(m_N[0]-h,u_2L);
       u_N = Bulk_element_mesh_pt->boundary_node_pt(b,n)->value(0);
       (*u_fn)(m_N[0]+h,u_2R);      
       (*dudn_fn)(m_N[0]-0.5*h,dudn_L);
       (*dudn_fn)(m_N[0]+0.5*h,dudn_R);
      }
     // compute
     d2udm_t2 = (u_2L+u_2R-2*u_N)/(h*h);
     ddm_tdudn = (dudn_R-dudn_L)/h;

     // get dudn at the node
     double dudn;
     (*dudn_fn)(m_N[0],dudn);

     // compute d2u/ds_t2
     double d2uds_t2 = m_N[1]*m_N[1]*d2udm_t2;

     // get duds_t
     double duds_t = Bulk_element_mesh_pt->
      boundary_node_pt(b,n)->value(2-s_fixed_index);

     // get du/dt
     double dudt = ds_tdt*duds_t;

     // compute d2u/dndt
     double d2udndt = ds_tdt= dtds_t*m_N[1]*ddm_tdudn;

     // compute d2udt2
     double dtds_nd2udt2 = edge_sign*(dxds_t[0]*dxds_n[1]-
                                      dxds_n[0]*dxds_t[1])*
      (ds_tdt*(d2udndt - ds_ndn*(d2s_tds_ndt*dudt+
                                 ds_tdt*d2uds_t2)));
     
     // compute dds_n(dudt)
     double dds_ndudt = dtds_nd2udt2 + dnds_n*d2udndt;

     // compute du/ds_n
     double duds_n = dnds_n*dudn + dtds_n*ds_tdt*duds_t;

     // compute d2u/ds_nds_t
     double d2uds_nds_t = d2tds_nds_t*dudt+dtds_t*dds_ndudt;

     // pin du/ds_n dof and set value
     Bulk_element_mesh_pt->boundary_node_pt(b,n)->pin(1+s_fixed_index);
     Bulk_element_mesh_pt->boundary_node_pt(b,n)->
      set_value(1+s_fixed_index,duds_n);

     // pin d2u/ds_nds_t dof and set value
     Bulk_element_mesh_pt->boundary_node_pt(b,n)->pin(3);
     Bulk_element_mesh_pt->boundary_node_pt(b,n)->set_value(3,d2uds_nds_t);
    }
  }
}



//=============================================================================
/// \short Imposes a 'free' edge. Imposes the prescribed Neumann BCs 
/// laplacian(u) and dlaplacian(u)/dn with flux edge elements
//=============================================================================
template<unsigned DIM>
void BiharmonicProblem<DIM>::
set_neumann_boundary_condition(const unsigned &b, 
                               BiharmonicFluxElement<2>::FluxFctPt 
                               flux0_fct_pt,
                               BiharmonicFluxElement<2>::FluxFctPt 
                               flux1_fct_pt)
{

 // if the face element mesh pt does not exist then build it
 if (Face_element_mesh_pt == 0)
  {
   Face_element_mesh_pt = new Mesh();
  }

 // How many bulk elements are adjacent to boundary b?
 unsigned n_element = Bulk_element_mesh_pt->nboundary_element(b);
 
 // Loop over the bulk elements adjacent to boundary b?
 for(unsigned e=0;e<n_element;e++)
  {
   
   // Get pointer to the bulk element that is adjacent to boundary b
   FiniteElement* bulk_elem_pt = 
    dynamic_cast<FiniteElement* >(Bulk_element_mesh_pt->boundary_element_pt(b,e));
   
   // What is the face index along the boundary
   int face_index = Bulk_element_mesh_pt->face_index_at_boundary(b,e);
   
   // Build the corresponding prescribed-flux element
   BiharmonicFluxElement<2> *flux_element_pt = new 
    BiharmonicFluxElement<2>
    (bulk_elem_pt, face_index, b);

     
   // pass the flux BC pointers to the flux elements
   flux_element_pt->flux0_fct_pt() = flux0_fct_pt;
   if (flux1_fct_pt != 0)
    {
     flux_element_pt->flux1_fct_pt() = flux1_fct_pt;       
    }

   //Add the prescribed-flux element to the mesh
   Face_element_mesh_pt->add_element_pt(flux_element_pt);
  } 
}


//=============================================================================
/// \short documents the solution, and if an exact solution is provided, then 
/// the error between the numerical and exact solution is presented
//=============================================================================
template<unsigned DIM>
void BiharmonicProblem<DIM>::
doc_solution(DocInfo& doc_info, 
             FiniteElement::SteadyExactSolutionFctPt exact_soln_pt)
{  
 std::ofstream some_file;
 std::ostringstream filename;
 
 // Number of plot points: npts x npts
 unsigned npts=5;

 // Output solution 
 filename << doc_info.directory() << "/soln_" << doc_info.number() << ".dat";
 some_file.open(filename.str().c_str());
 Bulk_element_mesh_pt->output(some_file,npts);
 some_file.close();
 
 // if the exact solution is not provided
 if (exact_soln_pt != 0)
  {

   // Output exact solution
   filename.str("");
   filename << doc_info.directory() << "/exact_soln_"
            << doc_info.number() << ".dat";
   some_file.open(filename.str().c_str()); 
   Bulk_element_mesh_pt->output_fct(some_file,npts, exact_soln_pt);
   some_file.close();
     
   // Doc error and return of the square of the L2 error
   double error,norm;
   filename.str("");
   filename << doc_info.directory() << "/error_" 
            << doc_info.number() << ".dat";
   some_file.open(filename.str().c_str());
   Bulk_element_mesh_pt->compute_error(some_file, exact_soln_pt, error, norm);
   some_file.close();
   
   // Doc L2 error and norm of solution
   oomph_info << "\nNorm of error   : " << sqrt(error) << std::endl; 
   oomph_info << "Norm of solution: " << sqrt(norm) << std::endl << std::endl;
  }
}




//=============================================================================
/// \short Computes the elemental residual vector and the elemental jacobian
/// matrix if JFLAG = 0
/// Imposes the equations :  du/ds_n = dt/ds_n * ds_t/dt * du/dt
//=============================================================================
void BiharmonicFluidBoundaryElement::
fill_in_generic_residual_contribution_biharmonic_boundary(
 Vector<double> &residual, 
 DenseMatrix<double>& jacobian, 
 unsigned JFLAG)
{
 
 // dof # corresponding to d/ds_n
 unsigned k_normal = 1+S_fixed_index;
 
 // dof # corresponding to d/ds_t
 unsigned k_tangential = 2-S_fixed_index;

 // vectors for dx_i/ds_n and dx_i/ds_t
 Vector<double> dxds_n(2);
 Vector<double> dxds_t(2);
 dxds_n[0] = this->node_pt(0)->x_gen(1+S_fixed_index,0);
 dxds_n[1] = this->node_pt(0)->x_gen(1+S_fixed_index,1);
 dxds_t[0] = this->node_pt(0)->x_gen(2-S_fixed_index,0);
 dxds_t[1] = this->node_pt(0)->x_gen(2-S_fixed_index,1);
 
 // vector for d2xi/ds_n ds_t
 Vector<double> d2xds_nds_t(2);
 d2xds_nds_t[0] = this->node_pt(0)->x_gen(3,0);
 d2xds_nds_t[1] = this->node_pt(0)->x_gen(3,1);

 // compute dt/ds_n
 double dtds_n = ((dxds_n[0]*dxds_t[0])+(dxds_n[1]*dxds_t[1]))
  /sqrt(dxds_t[0]*dxds_t[0]+dxds_t[1]*dxds_t[1]);
 
 // compute ds_t/dt
 double ds_tdt = 1/sqrt(dxds_t[0]*dxds_t[0]+dxds_t[1]*dxds_t[1]);   

 // determine the local equation number
 int local_eqn_number = this->nodal_local_eqn(0,k_normal);
 
 // if local equation number equal to -1 then its a boundary node(pinned)
 if (local_eqn_number >= 0)
  {

   // additions to residual for duds_n
   residual[local_eqn_number] += (this->node_pt(0)->value(k_normal) - 
                                  dtds_n*ds_tdt * 
                                  this->node_pt(0)->value(k_tangential));

   // if contributions to jacobian are required
   if (JFLAG == 1)
    {
     
     // additions to jacobian for du/ds_n
     int local_dof_number = this->nodal_local_eqn(0,k_normal); 
     if (local_dof_number >= 0) 
      {
       jacobian(local_eqn_number,local_dof_number) += 1.0;
      }
     local_dof_number = this->nodal_local_eqn(0,k_tangential); 
     if (local_dof_number >= 0) 
      {  
       jacobian(local_eqn_number,local_dof_number) -= dtds_n*ds_tdt;
      }
    }
  }
}






//=============================================================================
/// \short Imposes a solid boundary - no flow into boundary or along boundary
/// v_n = 0 and v_t = 0. User must presribe the streamfunction psi to ensure 
/// dpsi/dt = 0 is imposed at all points on the boundary and not just at the 
/// nodes
//=============================================================================
template<unsigned DIM>
void BiharmonicFluidProblem<DIM>::
impose_solid_boundary_on_edge(const unsigned& b, const double& psi)
{  
 // number of nodes on boundary b
 unsigned n_node = mesh_pt()->nboundary_node(b);
 
 // loop over nodes on boundary b
 for (unsigned n = 0; n < n_node; n++)
  {
   // loop over DOFs to be pinned du/ds_n, du/ds_t and d2u/ds_nds_t
   for ( unsigned k = 1; k < 4; k++)
    {
     // pin and zero DOF k 
     mesh_pt()->boundary_node_pt(b,n)->pin(k);   
     mesh_pt()->boundary_node_pt(b,n)->set_value(k,0.0);
    }
   mesh_pt()->boundary_node_pt(b,n)->pin(0);   
   mesh_pt()->boundary_node_pt(b,n)->set_value(0,psi);   
  }
}



//=============================================================================
/// \short Impose a traction free edge - i.e. v_t = 0 or dpsi/dn = 0. In 
/// general dpsi/dn = 0 can only be imposed using equation elements to set 
/// the DOFs dpsi/ds_n, however in the special case of  dt/ds_n = 0, then 
/// dpsi/ds_n = 0 and can be imposed using pinning - this is handled 
/// automatically in this function. For a more detailed description of the 
/// equations see the description of the class : BiharmonicFluidBoundaryElement
//=============================================================================
template<unsigned DIM>
void BiharmonicFluidProblem<DIM>::
impose_traction_free_edge(const unsigned& b)
{
 // fixed faced index for boundary
 int face_index = mesh_pt()->face_index_at_boundary(b,0);

 //Need to get the s_fixed_index
 unsigned s_fixed_index = 0;
 switch(face_index)
  {
  case -1:
  case 1:
   s_fixed_index = 0;
   break;

  case -2:
  case 2:
   s_fixed_index = 1;
   break;
   
  default:
   throw OomphLibError("Face Index not +/-1 or +/-2: Need 2D QElements",
                       OOMPH_CURRENT_FUNCTION,
                       OOMPH_EXCEPTION_LOCATION);
  }
 
 // create a point to a hijacked biharmonic element
 Hijacked<BiharmonicElement<2> >* hijacked_element_pt;

 // vectors for dx_i/ds_n and dx_i/ds_t
 Vector<double> dxds_n(2);
 Vector<double> dxds_t(2);

 // number of nodes along edge
 unsigned n_node = mesh_pt()->nboundary_node(b);

 // loop over boudnary nodes
 for (unsigned n = 0; n < n_node; n++)
  {

   // get dx_i/ds_t and dx_i/ds_n at node n
   dxds_n[0] = mesh_pt()->boundary_node_pt(b,n)->x_gen(1+s_fixed_index,0);
   dxds_n[1] = mesh_pt()->boundary_node_pt(b,n)->x_gen(1+s_fixed_index,1);
   dxds_t[0] = mesh_pt()->boundary_node_pt(b,n)->x_gen(2-s_fixed_index,0);
   dxds_t[1] = mesh_pt()->boundary_node_pt(b,n)->x_gen(2-s_fixed_index,1);

   // compute dt/ds_n
   double dtds_n = ((dxds_n[0]*dxds_t[0])+(dxds_n[1]*dxds_t[1]))
    /sqrt(dxds_t[0]*dxds_t[0]+dxds_t[1]*dxds_t[1]);

   // if dt/ds_n = 0 we can impose the traction free edge at this node by 
   // pinning dpsi/ds_n = 0, otherwise the equation elements 
   // BiharmonicFluidBoundaryElement are used
   if (dtds_n == 0.0)
    {
     // pin dpsi/dn
     mesh_pt()->boundary_node_pt(b,n)->pin(1+s_fixed_index);
     mesh_pt()->boundary_node_pt(b,n)->set_value(1+s_fixed_index,0.0);
    }

   // otherwise impose equation elements
   else
    {
     // hijack DOFs in element either side of node
     // boundary 0 
     if (b == 0)
      {
       // hijack DOFs in left element
       if (n > 0)
        {
         hijacked_element_pt =
          dynamic_cast<Hijacked<BiharmonicElement<2> > *>
          (mesh_pt()->boundary_element_pt(b,n-1));
         delete hijacked_element_pt->hijack_nodal_value(1,1+s_fixed_index);
        }
       // hijack DOFs in right element
       if (n < (n_node-1))
        {
         hijacked_element_pt =
          dynamic_cast<Hijacked<BiharmonicElement<2> > *>
          (mesh_pt()->boundary_element_pt(b,n));
         delete hijacked_element_pt->hijack_nodal_value(0,1+s_fixed_index);
        }
      }
     // boundary 1
     else if (b == 1)
      {
       // hijack DOFs in left element
       if (n > 0)
        {
         hijacked_element_pt =
          dynamic_cast<Hijacked<BiharmonicElement<2> > *>
          (mesh_pt()->boundary_element_pt(b,n-1));
         delete hijacked_element_pt->hijack_nodal_value(3,1+s_fixed_index);
        }
       // hijack DOFs in right element
       if (n < n_node-1)
        {
         hijacked_element_pt =
          dynamic_cast<Hijacked<BiharmonicElement<2> > *>
          (mesh_pt()->boundary_element_pt(b,n));
         delete hijacked_element_pt->hijack_nodal_value(1,1+s_fixed_index);
        }
      }

     // boundary 2
     else if (b == 2)
      {
       // hijack DOFs in left element
       if (n > 0)
        {
         hijacked_element_pt =
          dynamic_cast<Hijacked<BiharmonicElement<2> > *>
          (mesh_pt()->boundary_element_pt(b,n-1));
         delete hijacked_element_pt->hijack_nodal_value(3,1+s_fixed_index);
        }
       if (n < n_node-1)
        {
         // hijack DOFs in right element
         hijacked_element_pt =
          dynamic_cast<Hijacked<BiharmonicElement<2> > *>
          (mesh_pt()->boundary_element_pt(b,n));
         delete hijacked_element_pt->hijack_nodal_value(2,1+s_fixed_index);
        }
      }
     // boundary 3
     else if  (b == 3)
      {
       // hijack DOFs in left element
       if (n > 0)
        {
         hijacked_element_pt =
          dynamic_cast<Hijacked<BiharmonicElement<2> > *>
          (mesh_pt()->boundary_element_pt(b,n-1));
         delete hijacked_element_pt->hijack_nodal_value(2,1+s_fixed_index);
        }
       if (n < n_node-1)
        {
         // hijack DOFs in right element
         hijacked_element_pt =
          dynamic_cast<Hijacked<BiharmonicElement<2> > *>
          (mesh_pt()->boundary_element_pt(b,n));
         delete hijacked_element_pt->hijack_nodal_value(0,1+s_fixed_index);
        }
      }

     // create the boundary point element
     BiharmonicFluidBoundaryElement* boundary_point_element_pt =   
      new BiharmonicFluidBoundaryElement(mesh_pt()->boundary_node_pt(b,n),
                                         s_fixed_index);

     // add element to mesh
     mesh_pt()->add_element_pt(boundary_point_element_pt);

     // increment number of non bulk elements
     Npoint_element++;
    }
  }
}


//=============================================================================
/// \short Impose a prescribed fluid flow comprising the velocity normal to 
/// the boundary (u_imposed_fn[0]) and the velocity tangential to the 
/// boundary (u_imposed_fn[1])
//=============================================================================
template<unsigned DIM>
void BiharmonicFluidProblem<DIM>::
impose_fluid_flow_on_edge(const unsigned& b,FluidBCFctPt u_imposed_fn)
{
 
 // number of nodes on boundary b
 unsigned n_node = mesh_pt()->nboundary_node(b);
 
 // fixed faced index for boundary
 int face_index = mesh_pt()->face_index_at_boundary(b,0);

 //Need to get the s_fixed_index
 unsigned s_fixed_index = 0;
 //Also set the edge sign
 int edge_sign= 0;

 switch(face_index)
  {
  case -1:
   s_fixed_index = 0;
   edge_sign = -1;
   break;
   
  case 1:
   s_fixed_index = 0;
   edge_sign = 1;
   break;

  case -2:
   s_fixed_index = 1;
   edge_sign = 1;
   break;
   
  case 2:
   s_fixed_index = 1;
   edge_sign = -1;
   break;
   
  default:
   throw OomphLibError("Face Index not +/-1 or +/-2: Need 2D QElements",
                       OOMPH_CURRENT_FUNCTION,
                       OOMPH_EXCEPTION_LOCATION);
  }

 
 // node position along edge b in macro element boundary representation 
 // [-1,1]
 Vector<double> s(1); 
   
 // finite difference step
 const double h = 10e-8;
 
 // loop over nodes on boundary b                                         
 for (unsigned n = 0; n < n_node; n++)              
  {
   
   // get m_t and dm_t/ds_t for this node
   Vector<double> m_N(2);
   mesh_pt()->boundary_node_pt(b,n)->get_coordinates_on_boundary(b,m_N);
   
   // vectors for dx_i/ds_n and dx_i/ds_t
   Vector<double> dxds_n(2);
   Vector<double> dxds_t(2);
   dxds_n[0] = mesh_pt()->boundary_node_pt(b,n)->x_gen(1+s_fixed_index,0);
   dxds_n[1] = mesh_pt()->boundary_node_pt(b,n)->x_gen(1+s_fixed_index,1);
   dxds_t[0] = mesh_pt()->boundary_node_pt(b,n)->x_gen(2-s_fixed_index,0);
   dxds_t[1] = mesh_pt()->boundary_node_pt(b,n)->x_gen(2-s_fixed_index,1);
   
   // vector for d2xi/ds_n ds_t
   Vector<double> d2xds_nds_t(2);
   d2xds_nds_t[0] = mesh_pt()->boundary_node_pt(b,n)->x_gen(3,0);
   d2xds_nds_t[1] = mesh_pt()->boundary_node_pt(b,n)->x_gen(3,1);
   
   // compute dn/ds_n
   double dnds_n = ((dxds_n[0]*dxds_t[1])-(dxds_n[1]*dxds_t[0]))
    / (sqrt(dxds_t[0]*dxds_t[0]+dxds_t[1]*dxds_t[1])*edge_sign);
   
   // compute dt/ds_n
   double dtds_n = ((dxds_n[0]*dxds_t[0])+(dxds_n[1]*dxds_t[1]))
    /sqrt(dxds_t[0]*dxds_t[0]+dxds_t[1]*dxds_t[1]);

   // compute dt/ds_t
   double dtds_t = sqrt(dxds_t[0]*dxds_t[0]+dxds_t[1]*dxds_t[1]);

   // compute d2t/ds_nds_t
   double d2tds_nds_t = (dxds_t[0]*d2xds_nds_t[0] + dxds_t[1]*d2xds_nds_t[1])
    /sqrt(dxds_t[0]*dxds_t[0] + dxds_t[1]*dxds_t[1]);
   
   // get imposed velocities
   Vector<double> u(2);
   (*u_imposed_fn)(m_N[0],u);
   u[0]*=edge_sign;
   u[1]*=-edge_sign;
   
   // compute  d/dm_t(dpsidn) by finite difference
   double ddm_tdudn = 0;
   double ddm_tdudt = 0;
   Vector<double> u_L(2);
   Vector<double> u_R(2);
   // evaluate dudn_fn about current node
   if (n == 0)
    {
     (*u_imposed_fn)(m_N[0],u_L);
     (*u_imposed_fn)(m_N[0]+h,u_R);
    }
   else if (n == n_node-1)
    {
     (*u_imposed_fn)(m_N[0]-h,u_L);
     (*u_imposed_fn)(m_N[0],u_R);
    }
   else
    {   
     (*u_imposed_fn)(m_N[0]-0.5*h,u_L);
     (*u_imposed_fn)(m_N[0]+0.5*h,u_R);
    }   
   // compute
   ddm_tdudn = (u_R[1]-u_L[1])/h;   
   ddm_tdudt = (u_R[0]-u_L[0])/h;

   // compute du/ds_t
   double duds_t = dtds_t*u[0];

   // compute du/ds_n
   double duds_n = dnds_n*u[1] + dtds_n*u[0];
 
   // compute d2u/ds_n ds_t
   double d2uds_nds_t = dnds_n*m_N[1]*ddm_tdudn + d2tds_nds_t*u[0] 
    + dtds_n*m_N[1]*ddm_tdudt;
   
   // pin du/ds_n dof and set value
   mesh_pt()->boundary_node_pt(b,n)->pin(1+s_fixed_index);
   mesh_pt()->boundary_node_pt(b,n)->set_value(1+s_fixed_index,duds_n);

   // pin du/ds_t dof and set value
   mesh_pt()->boundary_node_pt(b,n)->pin(2-s_fixed_index);
   mesh_pt()->boundary_node_pt(b,n)->set_value(2-s_fixed_index,duds_t);   

   // pin du/ds_t dof and set value
   mesh_pt()->boundary_node_pt(b,n)->pin(3);
   mesh_pt()->boundary_node_pt(b,n)->set_value(3,d2uds_nds_t);  
  }
}


//=============================================================================
/// \short documents the solution, and if an exact solution is provided, then 
/// the error between the numerical and exact solution is presented
//=============================================================================
template<unsigned DIM>
void BiharmonicFluidProblem<DIM>::
doc_solution(DocInfo& doc_info, 
             FiniteElement::SteadyExactSolutionFctPt exact_soln_pt)
{  
 // create an output stream
 std::ofstream some_file;
 std::ostringstream filename;

 // Number of plot points: npts x npts
 unsigned npts=5;

 // Output solution 
 filename << doc_info.directory() << "/soln_"
          << doc_info.label() << ".dat";
 some_file.open(filename.str().c_str());
 mesh_pt()->output(some_file,npts);
 some_file.close();

  // Output fluid velocity solution
 filename.str("");
 filename << doc_info.directory() << "/soln_velocity_" 
          << doc_info.label() << ".dat";
 some_file.open(filename.str().c_str());
 unsigned n_element = mesh_pt()->nelement();
 for (unsigned i = 0; i < n_element-Npoint_element; i++)
  {
   BiharmonicElement<2>* biharmonic_element_pt =  
    dynamic_cast<BiharmonicElement<2>* >(mesh_pt()->element_pt(i));
   biharmonic_element_pt->output_fluid_velocity(some_file,npts);
  }
 some_file.close();

 // if the exact solution is not provided
 if (exact_soln_pt != 0)
  {

   // Output exact solution
   filename.str("");
   filename << doc_info.directory() << "/exact_soln_"
            << doc_info.label() << ".dat";
   some_file.open(filename.str().c_str()); 
   mesh_pt()->output_fct(some_file,npts, exact_soln_pt);
   some_file.close();
     
   // Doc error and return of the square of the L2 error
   double error,norm;
   filename.str("");
   filename << doc_info.directory() << "/error_"
            << doc_info.label() << ".dat";
   some_file.open(filename.str().c_str());
   mesh_pt()->compute_error(some_file, exact_soln_pt, error, norm);
   some_file.close();
   
   // Doc L2 error and norm of solution
   oomph_info << "\nNorm of error   : " << sqrt(error) << std::endl; 
   oomph_info << "Norm of solution: " << sqrt(norm) << std::endl << std::endl;
  }
}

// ensure build
template class BiharmonicFluidProblem<2>;
template class BiharmonicProblem<2>;

}








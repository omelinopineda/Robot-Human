
#include <leg-odometry/QuaternionLib.h>



Eigen::Vector3d q2e_new(const Eigen::Quaterniond q) {
  Eigen::Vector3d E;

  //std::cout << "new q2e\n";

  double roll_a = 2. * (q.w()*q.x() + q.y()*q.z());
  double roll_b = 1. - 2. * (q.x()*q.x() + q.y()*q.y());
  E(0) = atan2 (roll_a, roll_b);

  double pitch_sin = 2. * (q.w()*q.y() - q.z()*q.x());
  E(1) = asin (pitch_sin);

  double yaw_a = 2. * (q.w()*q.z() + q.x()*q.y());
  double yaw_b = 1. - 2. * (q.y()*q.y() + q.z()*q.z());
  E(2) = atan2 (yaw_a, yaw_b);


  return E;
}

Eigen::Quaterniond C2q(const Eigen::Matrix3d C) {

	//std::cout << "new C2q\n";

	double rot[9];
	double quat[4];

	rot[0] = C(0,0);
	rot[1] = C(0,1);
	rot[2] = C(0,2);
	rot[3] = C(1,0);
	rot[4] = C(1,1);
	rot[5] = C(1,2);
	rot[6] = C(2,0);
	rot[7] = C(2,1);
	rot[8] = C(2,2);

    quat[0] = 0.5*sqrt(rot[0]+rot[4]+rot[8]+1);

    if (fabs(quat[0]) > 1e-8) {
        double w4 = 1.0/(4.0*quat[0]);
      quat[1] = (rot[7]-rot[5]) * w4;
      quat[2] = (rot[2]-rot[6]) * w4;
      quat[3] = (rot[3]-rot[1]) * w4;
    }
    else {
      quat[1] = sqrt(fabs(-0.5*(rot[4]+rot[8])));
      quat[2] = sqrt(fabs(-0.5*(rot[0]+rot[8])));
      quat[3] = sqrt(fabs(-0.5*(rot[0]+rot[4])));
    }

    /*LSF: I may be missing something but this didn't work for me until I divided by the magnitude instead:
      double norm = quat[0]*quat[0] + quat[1]*quat[1] + quat[2]*quat[2] +
                       quat[3]*quat[3];
    if (fabs(norm) < 1e-10)
    return -1; */
    double norm = sqrt(quat[0]*quat[0] + quat[1]*quat[1] + quat[2]*quat[2] +
                       quat[3]*quat[3]);

    norm = 1/norm;
    quat[0] *= norm;
    quat[1] *= norm;
    quat[2] *= norm;
    quat[3] *= norm;

    Eigen::Quaterniond ret;
    ret.w() = quat[0];
    ret.x() = quat[1];
    ret.y() = quat[2];
    ret.z() = quat[3];

    return ret;
  }

Eigen::Matrix3d q2C(const Eigen::Quaterniond q_) {
	//std::cout << "new q2C\n";

	Eigen::Matrix3d C;

	  double w = q_.w();
	  double x = q_.x();
	  double y = q_.y();
	  double z = q_.z();

		// DRC bot_frames

		//double quat[4];
		double rot[9];

		/*quat[0] = q_.w();
		quat[1] = q_.x();
		quat[2] = q_.y();
		quat[3] = q_.z();*/

		double norm = w*w + x*x + y*y + z*z;
		if (fabs(norm) < 0.8) {
			std::cerr << "QuaternionLib::q2C -- not a unit quaternion\n";
			return Eigen::Matrix3d::Identity();
		}

		norm = 1./norm;
		w = w*norm;
		x = x*norm;
		y = y*norm;
		z = z*norm;

		double x2 = x*x;
		double y2 = y*y;
		double z2 = z*z;
		double w2 = w*w;
		double xy = 2*x*y;
		double xz = 2*x*z;
		double yz = 2*y*z;
		double wx = 2*w*x;
		double wy = 2*w*y;
		double wz = 2*w*z;

		rot[0] = w2+x2-y2-z2;  rot[1] = xy-wz;  rot[2] = xz+wy;
		rot[3] = xy+wz;  rot[4] = w2-x2+y2-z2;  rot[5] = yz-wx;
		rot[6] = xz-wy;  rot[7] = yz+wx;  rot[8] = w2-x2-y2+z2;

		for (int i=0;i<3;i++) {
			for (int j=0;j<3;j++) {
				C(i,j) = rot[3*i+j]; // Eigen is column major
			}
		}

	  //std::cout << C <<std::endl;

	  return C;
}


Eigen::Vector3d C2e(const Eigen::Matrix3d C) {

	  Eigen::Quaterniond q;

	  q = C2q(C);

	  q.normalize();

	  //std::cout << "Q at this point is: " << q.w() << ", " << q.x() << ", " << q.y() << ", " << q.z() << std::endl;

	  Eigen::Vector3d ret;
	  ret = q2e_new(q);

	  return ret;
}

Eigen::Quaterniond e2q(const Eigen::Vector3d &E) {
	  //Eigen::Quaterniond q;

	  //std::cout << "new e2q" << std::endl;

	  //return C2q(e2C(E));

	  Eigen::Quaterniond q_return;
	  double roll = E(0), pitch = E(1), yaw = E(2);

   double halfroll = roll / 2.;
   double halfpitch = pitch / 2.;
   double halfyaw = yaw / 2.;

   double sin_r2 = sin (halfroll);
   double sin_p2 = sin (halfpitch);
   double sin_y2 = sin (halfyaw);

   double cos_r2 = cos (halfroll);
   double cos_p2 = cos (halfpitch);
   double cos_y2 = cos (halfyaw);

   q_return.w() = cos_r2 * cos_p2 * cos_y2 + sin_r2 * sin_p2 * sin_y2;
   q_return.x() = sin_r2 * cos_p2 * cos_y2 - cos_r2 * sin_p2 * sin_y2;
   q_return.y() = cos_r2 * sin_p2 * cos_y2 + sin_r2 * cos_p2 * sin_y2;
   q_return.z() = cos_r2 * cos_p2 * sin_y2 - sin_r2 * sin_p2 * cos_y2;

   return q_return;
}

Eigen::Matrix3d e2C(Eigen::Vector3d Ec) {


	Eigen::Matrix3d C;

	C = q2C(e2q(Ec));

	return C;
	/*

	// untested

	  double st, sp, sps;
	  double ct, cp, cps;

	  //std::cout << "e2C\n";

	  sp = sin(Ec(0));
	  st = sin(Ec(1));
	  sps = sin(Ec(2));

	  cp = cos(Ec(0));
	  ct = cos(Ec(1));
	  cps = cos(Ec(2));

	  C(0,0) = cps*ct;
	  C(0,1) = sps*ct;
	  C(0,2) = -st;

	  C(1,0) = -sps*cp + cps*st*sp;
	  C(1,1) = cps*cp + sps * st * sp;
	  C(1,2) = ct*sp;

	  C(2,0) = sps * sp + cps * st * cp;
	  C(2,1) = -cps*sp + sp * st * cp;
	  C(2,2) = ct * cp;


	  */
  }

void skew(Eigen::Vector3d const &v_, Eigen::Matrix<double,3,3> &skew)
{

  skew << 0., -v_(2), v_(1),
		  v_(2), 0., -v_(0),
		  -v_(1), v_(0), 0.;

  return;
}


// Quaternion product for [scalar vector] quaternion -- Purposefully does not renormalize.
Eigen::Quaterniond qprod(const Eigen::Quaterniond &_b, const Eigen::Quaterniond &_a) {

	//	B = [b(1), -b(2), -b(3), -b(4);...
	//	     b(2),  b(1), -b(4),  b(3);...
	//	     b(3),  b(4),  b(1), -b(2);...
	//	     b(4), -b(3),  b(2),  b(1)];
	//
	//	 q = B*c;

	Eigen::Quaterniond result;
	result.setIdentity();

	result.w() = _b.w()*_a.w() - _b.x()*_a.x() - _b.y()*_a.y() - _b.z()*_a.z();
	result.x() = _b.x()*_a.w() + _b.w()*_a.x() - _b.z()*_a.y() + _b.y()*_a.z();
	result.y() = _b.y()*_a.w() + _b.z()*_a.x() + _b.w()*_a.y() - _b.x()*_a.z();
	result.z() = _b.z()*_a.w() - _b.y()*_a.x() + _b.x()*_a.y() + _b.w()*_a.z();

	return result;
}

Eigen::Vector3d qrot(const Eigen::Quaterniond &_aQb, const Eigen::Vector3d &_v) {

	//	V_A = [0;v_a];
	//
	//	V_B = qprod( aQb,  qprod(V_A,qconj(aQb))  );
	//
	//	v_b = V_B(2:4);

	Eigen::Quaterniond Va;
	Eigen::Quaterniond Vb;

	Va.w() = 0.;
	Va.x() = _v(0);
	Va.y() = _v(1);
	Va.z() = _v(2);

	Vb = qprod(_aQb, qprod(Va, _aQb.conjugate()));

	return Eigen::Vector3d(Vb.x(), Vb.y(), Vb.z());
}

Eigen::Quaterniond exmap(const Eigen::Vector3d &dE_l, const Eigen::Quaterniond &lQb) {
	//		w_dt = w*dt; -- This is dE_l (delta rotation in the local frame)
	//		w_norm = norm(w);
	//		w_norm_dt = w_norm*dt;
	//
	//		if (w_norm>1E-6) % 1E-7 is chosen because double numerical LSB is around 1E-18 for nominal values [-pi..pi]
	//		    % and (1E-7)^2 is at 1E-14, but 1E-7 rad/s is 0.02 deg/h
	//		    r = [cos(w_norm*dt/2);...
	//		         w./w_norm*sin(w_norm*dt/2)];
	//
	//		    aQb_k0 = qprod(r,aQb_k1);
	//		else
	//		    r = [cos(w_norm_dt/2);...
	//		         w_dt*(0.5-w_norm_dt*w_norm_dt/48)];
	//
	//		    aQb_k0 = qprod(r,aQb_k1);
	//		end
	//
	//		if (abs(1-norm(aQb_k0))>1E-13)
	//		    disp 'zeroth_int_Quat_closed_form -- normalizing time propagated quaternion'
	//		    aQb_k0 = aQb_k0./norm(aQb_k0);
	//		end


	double dE_lNorm;
	dE_lNorm = dE_l.norm();

	Eigen::Quaterniond r;
	Eigen::Quaterniond result;
	r.setIdentity();
	result.setIdentity();

	double sindE;
	sindE = sin(0.5*dE_lNorm)/dE_lNorm;

	if (dE_lNorm>1E-7) { //% 1E-7 is chosen because double numerical LSB is around 1E-15 for nominal values [-pi..pi]
		r.w() = cos(0.5*dE_lNorm);
		r.x() = dE_l(0)*sindE;
		r.y() = dE_l(1)*sindE;
		r.z() = dE_l(2)*sindE;
	} else {
		double tmp;
		tmp = (0.5 - dE_lNorm*dE_lNorm*0.020833333333333333);

		r.w() = cos(0.5*dE_lNorm);
		r.x() = dE_l(0)*tmp;
		r.y() = dE_l(1)*tmp;
		r.z() = dE_l(2)*tmp;
	}

	result = qprod(r,lQb);

	if (abs(1-result.norm()) > 1E-13){
		std::cout << "OrientationComputer::exmap -- had to renormalize quaternion" << std::endl;
		result = result.normalized();
	}
	return result;
}

void printq(const std::string &_from, const Eigen::Quaterniond &_q) {
  std::cout << _from << " w " << _q.w() << ", " << _q.x() << ", " << _q.y() << ", " << _q.z() << std::endl;
}







namespace InertialOdometry 
{


class QuaternionLib {
  public:
	 EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    // The result is pushed back into a2 (overwrites what was in there)
    static void QuaternionProduct(double *a1, double *a2);
	static Eigen::Quaterniond QuaternionProduct_(const Eigen::Quaterniond &lhs, const Eigen::Quaterniond &rhs);

    // Result is pushed back by overwritting the argument variable
    static void QuaternionAdjoint(double *q);

    // q is the rotation quaternion
    // v is the vector to be rotated, assumed to be size 3
    // Result is pushed back by overwriting the v variable
    //static void VectorRotation(double *q, double *v);
    static void VectorRotation(Eigen::Vector4d q, Eigen::Vector3d &v);

    //This function is specific to NED and forward right down coordinate frames
    static Eigen::Quaterniond e2q(const Eigen::Vector3d &E);
    static void q2e(const Eigen::Vector4d &q_, Eigen::Vector3d &E);
    static void q2e(const Eigen::Quaterniond &q_, Eigen::Vector3d &E);
    static void q2e_(const Eigen::Vector4d &q_, Eigen::Vector3d &E);

    static Eigen::Vector3d q2e(const Eigen::Quaterniond &q_);
    static void quat_to_euler(Eigen::Quaterniond q, double& yaw, double& pitch, double& roll);

    //get the direction cosine matrix equivalent to quaternion (scalar, vector)
    static void q2C(Eigen::Vector4d const &q_, Eigen::Matrix<double,3,3> &C);
    static Eigen::Matrix<double,3,3> q2C(Eigen::Quaterniond const &q_);
    static Eigen::Quaterniond C2q(const Eigen::Matrix<double, 3, 3> &C);
    static int matrix_to_quat(const double rot[9], double quat[4]);
    static Eigen::Quaterniond bot_matrix_to_quat(const Eigen::Matrix3d &C); // Think this one may finally be the correct one

    // Places the 3x3 skew symmetric matrix of 3x1 vector v in skew
    static void skew(Eigen::Vector3d const &v_, Eigen::Matrix<double,3,3> &skew);

    static void e2C(Eigen::Vector3d const &Ec, Eigen::Matrix<double, 3, 3> &C);
    static Eigen::Matrix<double, 3, 3> e2C(const Eigen::Vector3d &E);

    static Eigen::Vector3d C2e(const Eigen::Matrix<double, 3, 3> &C__);

    //Rotate a 3 vector around only te yaw axis
    static Eigen::Vector3d Cyaw_rotate(const Eigen::Matrix<double, 3, 3> &C, const Eigen::Vector3d &v);

    static void printEulerAngles(std::string prefix, const Eigen::Isometry3d &isom);
    static void printQuaternion(std::string preamble, const Eigen::Quaterniond &q);
  };


  
  // The result is pushed back into a2 (overwrites what was in there)
  void QuaternionLib::QuaternionProduct(double *a1, double *a2)
  {  
    Eigen::Matrix4d b;
    Eigen::Vector4d c;
    Eigen::Vector4d r;

    b(0,0) = a1[0];
    b(0,1) = -a1[1];
    b(0,2) = -a1[2];
    b(0,3) = -a1[3];

    b(1,0) = a1[1];
    b(1,1) = a1[0];
    b(1,2) = -a1[3];
    b(1,3) = a1[2];

    b(2,0) = a1[2];
    b(2,1) = a1[3];
    b(2,2) = a1[0];
    b(2,3) = -a1[1];

    b(3,0) = a1[3];
    b(3,1) = -a1[2];
    b(3,2) = a1[1];
    b(3,3) = a1[0];

    for (int i=0;i<4;i++)
      c(i) = a2[i]; 

    r = b*c;

    for (int i=0;i<4;i++)
      a2[i] = r(i); 

  }

  // Result is pushed back by overwritting the argument variable 
  void QuaternionLib::QuaternionAdjoint(double *q)
  {
    for (int i=1;i<4;i++)
      q[i] = -q[i];
  }
  
  Eigen::Quaterniond QuaternionLib::QuaternionProduct_(const Eigen::Quaterniond &lhs, const Eigen::Quaterniond &rhs) {
	  // This function has not been tested extensively -- must ensure that the Eigen Column major scheme does not affect the creation of the Q matrix
	  
  	Eigen::Quaterniond result;
  	
  	Eigen::Vector4d q;
  	Eigen::Vector4d p;
  	Eigen::Vector4d res;
  	//q*p - MARS Lab, Trawny Roumeliotis - Quaternion Algebra Tutorial Tech Report
  	q << rhs.x(), rhs.y(), rhs.z(), rhs.w();
  	
  	
  	Eigen::Matrix<double,4,4> Q;
  	
  	Q << 	q(3), q(2), -q(1), q(0),
  		   -q(2), q(3), q(0), q(1),
  		   q(1), -q(0), q(3), q(2),
  		   -q(0), -q(1), -q(2), q(3);
  	
  	p << lhs.x(), lhs.y(), lhs.z(), lhs.w();
  	
  	res = Q*p;
  	
  	result.w() = res(3);
  	result.x() = res(0);
  	result.y() = res(1);
  	result.z() = res(2);
  	
  	return result;
  }

  // q is the rotation quaternion
  // v is the vector to be rotated, assumed to be size 3
  // Result is pushed back by overwriting the v variable
//  void QuaternionLib::VectorRotation(double *q, double *v)
  void QuaternionLib::VectorRotation(Eigen::Vector4d q, Eigen::Vector3d &v)
  {
    double v_[4];
    double q_[4];
    v_[0] = 0.0;

    //int i;

    for (int i=0;i<4;i++)
      q_[i] = q(i);
    
    for (int i=1;i<4;i++)
      v_[i] = v(i-1);

    QuaternionProduct(q_,v_);
    QuaternionAdjoint(q_);
    QuaternionProduct(v_,q_);

    // Put the result back in the pointer to push back to caller
    for (int i=1;i<4;i++)
      v(i-1) = q_[i];

    //std::cout << " QuaternionLib::VectorRotation happened." << std::endl;
    
  }
  
  void QuaternionLib::q2e(const Eigen::Quaterniond &q_, Eigen::Vector3d &E) {
	  
	  Eigen::Vector4d var;
	  //var << q_.w(), q_.x(), q_.y(), q_.z();
	  
	  var(0) = q_.w();
	  var(1) = q_.x();
	  var(2) = q_.y();
	  var(3) = q_.z();
	  	  
	  var.normalize();
	  
	  //std::cout << "q2e(Eigen::Q, Eigen::V3) convention not confirmed "<< var.transpose() << "\n";
	  
	  q2e(var, E);
  }

  Eigen::Vector3d QuaternionLib::q2e(const Eigen::Quaterniond &q) {
	  Eigen::Vector3d E;

	  std::cout << "q2e\n";

	  //std::cout << "quat is: " << q.w() << ", " << q.x() << ", " << q.y() << ", " << q.z() << std::endl;
	  q2e(q,E);
	  
	  return E;
  }
  
  //This function is specific to NED and forward right down coordinate frames
  void QuaternionLib::q2e_(const Eigen::Vector4d &q_, Eigen::Vector3d &E)
  {
  	//Euler = [phi;theta;psi];


  	double a,b,c,d;

  	a = q_(0); // scalar
  	b = q_(1);
  	c = q_(2);
  	d = q_(3);

  	/*
  	phi = atan2(2*(c*d + b*a),a^2-b^2-c^2+d^2);
  	theta = asin(-2*(b*d - a*c));
  	psi = atan2(2*(b*c + d*a),a^2+b^2-c^2-d^2);*/

  	/*
  	 * As Michael does it..
  	roll = atan2(2*(q0*q1+q2*q3), 1-2*(q1*q1+q2*q2));
  	pitch = asin(2*(q0*q2-q3*q1));
  	yaw = atan2(2*(q0*q3+q1*q2), 1-2*(q2*q2+q3*q3));
	*/
  	/*
  	//Equivalent test
  	E(0) = atan2(2.0*(c*d + b*a),1.0-2.0*(b*b+c*c));
  	E(1) = asin(-2.0*(b*d - a*c));
  	E(2) = atan2(2.0*(b*c + d*a),1.0-2.0*(c*c+d*d));
  	*/

  	// This should be numerically more stable than the 1-(^2+ ^2) version..
  	// seems to be good
  	E(0) = atan2(2.0*(c*d + b*a),a*a-b*b-c*c+d*d);
  	E(1) = asin(-2.0*(b*d - a*c));
  	E(2) = atan2(2.0*(b*c + d*a),a*a+b*b-c*c-d*d);

  	return;
  }
  
  
  void QuaternionLib::q2e(const Eigen::Vector4d &q, Eigen::Vector3d &E)
  {
	  std::cout << "q2e\n";
	  // Function comes from libbot for quaternion [scalar vector] to Euler [roll p y]
	  //std::cout << "q2e received q: " << q.transpose() << "\n";
	  double roll_a = 2. * (q(0)*q(1) + q(2)*q(3));
	  double roll_b = 1. - 2. * (q(1)*q(1) + q(2)*q(2));
      E(0) = atan2 (roll_a, roll_b);

      double pitch_sin = 2. * (q(0)*q(2) - q(3)*q(1));
      E(1) = asin (pitch_sin);

      double yaw_a = 2. * (q(0)*q(3) + q(1)*q(2));
      double yaw_b = 1. - 2. * (q(2)*q(2) + q(3)*q(3));
      E(2) = atan2 (yaw_a, yaw_b);
      
  }
  
  
  // This one comes from Maurice for DRC, but they seem the same. This one follows the non-Homogeneous form
  void QuaternionLib::quat_to_euler(Eigen::Quaterniond q, double& yaw, double& pitch, double& roll) {
    const double q0 = q.w();
    const double q1 = q.x();
    const double q2 = q.y();
    const double q3 = q.z();
    roll = atan2(2*(q0*q1+q2*q3), 1-2*(q1*q1+q2*q2));
    pitch = asin(2*(q0*q2-q3*q1));
    yaw = atan2(2*(q0*q3+q1*q2), 1-2*(q2*q2+q3*q3));
  }

  void QuaternionLib::skew(Eigen::Vector3d const &v_, Eigen::Matrix<double,3,3> &skew)
  {
	  
	  skew << 0., -v_(2), v_(1), 
	  		  v_(2), 0., -v_(0),
	  		  -v_(1), v_(0), 0.;
	  
	  return;
  }
  
  Eigen::Matrix<double,3,3> QuaternionLib::q2C(Eigen::Quaterniond const &q_) {
	  Eigen::Matrix<double,3,3> mat;
	  Eigen::Vector4d q_pass;
	  
	  mat.setZero();
	  q_pass.setZero();
	  
	  q_pass(0) = q_.w();
	  q_pass(1) = q_.x();
	  q_pass(2) = q_.y();
	  q_pass(3) = q_.z();
	  
	  q2C(q_pass, mat);
	  
	  return mat;
  }
  
  // Calculate the left hand rotation matrix
  void QuaternionLib::q2C(Eigen::Vector4d const &q_, Eigen::Matrix<double,3,3> &C)
  {
	  std::cout << "old q2C\n";

	  // scalar vector format
	  //http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
	  // 3/10/2013
	  const double w = q_(0);
	  const double x = q_(1);
	  const double y = q_(2);
	  const double z = q_(3);
	  
	  
	  if (false) {
		  // something is not working with this transform
		  // It seems the diagonal elements are specified first. Not following continuous numbering


		  C(0,0) = w*w + x*x - y*y - z*z;
		  C(0,1) = 2.*(x*y - w*z);
		  C(0,2) = 2.*(w*y + x*z);
		  C(1,0) = w*w - x*x + y*y - z*z;
		  C(1,1) = 2.*(x*y + w*z);
		  C(1,2) = 2.*(y*z - w*x);
		  C(2,0) = w*w - x*x - y*y + z*z;
		  C(2,1) = 2.*(x*z - w*y);
		  C(2,2) = 2.*(w*x + y*z);

		  // confirm that this function generates a to b or b to a rotation, this function may need a transpose to generate the correct matrix
		  //std::cout << "TODO: QuaternionLib::q2C(..) is UNTESTED" << std::endl;

		  // TODO -- confirm that the rows and columns of the rotation matrix that is computed by this function maintains the DCM unity constraints
	  } else {

		// DRC bot_frames

		double quat[4];
		double rot[9];

		quat[0] = q_(0);
		quat[1] = q_(1);
		quat[2] = q_(2);
		quat[3] = q_(3);


		double norm = quat[0]*quat[0] + quat[1]*quat[1] + quat[2]*quat[2] + quat[3]*quat[3];
		if (fabs(norm) < 1e-10) {
			std::cerr << "QuaternionLib::q2C -- not a unit quaternion\n";

		}

		norm = 1/norm;
		double x = quat[1]*norm;
		double y = quat[2]*norm;
		double z = quat[3]*norm;
		double w = quat[0]*norm;

		double x2 = x*x;
		double y2 = y*y;
		double z2 = z*z;
		double w2 = w*w;
		double xy = 2*x*y;
		double xz = 2*x*z;
		double yz = 2*y*z;
		double wx = 2*w*x;
		double wy = 2*w*y;
		double wz = 2*w*z;

		rot[0] = w2+x2-y2-z2;  rot[1] = xy-wz;  rot[2] = xz+wy;
		rot[3] = xy+wz;  rot[4] = w2-x2+y2-z2;  rot[5] = yz-wx;
		rot[6] = xz-wy;  rot[7] = yz+wx;  rot[8] = w2-x2-y2+z2;

		for (int i=0;i<3;i++) {
			for (int j=0;j<3;j++) {
				C(i,j) = rot[3*i+j]; // Eigen is column major
			}
		}
	  }
	  
	  //std::cout << C <<std::endl;

	  return;
  }
  Eigen::Quaterniond QuaternionLib::C2q(const Eigen::Matrix<double, 3, 3> &C) {
	  
	  //std::cout << "QuaternionLib::C2q -- Think there is something wrong with this function\n";
	  
	  Eigen::Quaterniond returnval;
	  
	  if (false) {
	  double scalar;
	  
	  returnval.setIdentity();
	  scalar = 0.;
	  
	  // before the conversion for column major related ot Eigen Matrix
	  //w = 0.5*sqrt(1 + C(1,1) + C(2,2) + C(3,3));
	  //x = 1/(4*w)*(C(3,2)-C(2,3));
	  //y = 1/(4*w)*(C(1,3)-C(3,1));
	  //z = 1/(4*w)*(C(2,1)-C(1,2));
	  //std::cout << "Converting:\n" << C << std::endl;
	  scalar = 0.5*sqrt(1 + C(0,0) + C(1,1) + C(2,2));
	  	  
	  returnval.w() = scalar;
	  returnval.x() = 1/(4*scalar)*(C(1,2)-C(2,1));
	  returnval.y() = 1/(4*scalar)*(C(2,0)-C(0,2));
	  returnval.z() = 1/(4*scalar)*(C(0,1)-C(1,0));
	  
	  } else {
	  
	  double C_[9] = {C(0,0), C(0,1), C(0,2), C(1,0), C(1,1), C(1,2), C(2,0), C(2,1), C(2,2)};
	  double q[4];
	  
	  // This comes form libbot
	  matrix_to_quat(C_, q);
	  
	  returnval.w() = q[0];
	  returnval.x() = q[1];
	  returnval.y() = q[2];
	  returnval.z() = q[3];
	  }
	  
	  return returnval;
  }
  
  double max(double a, double b) {
  	  return (a > b ? a : b);
    }
    
    double sign(double a) {
	  return (a>=0 ? 1. : -1);
    }
  
    double copysign(double a, double b) {
  	  return sign(b) * abs(a);
    }
    
    
  /* This does not create a normalized quaternion
	Eigen::Quaterniond QuaternionLib::bot_matrix_to_quat(const Eigen::Matrix3d &C) {
	  Eigen::Quaterniond q;
	  
	  q.setIdentity();
	
	  q.w() = sqrt( max( 0, 1. + C(0,0) + C(1,1) + C(2,2) ) ) / 2.;
	  q.x() = sqrt( max( 0, 1. + C(0,0) - C(1,1) - C(2,2) ) ) / 2.;
	  q.y() = sqrt( max( 0, 1. - C(0,0) + C(1,1) - C(2,2) ) ) / 2.;
	  q.z() = sqrt( max( 0, 1. - C(0,0) - C(1,1) + C(2,2) ) ) / 2.;
	  q.x() = copysign( q.x(), C(1,2) - C(2,1) );
	  q.y() = copysign( q.y(), C(2,0) - C(0,2) );
	  q.z() = copysign( q.z(), C(0,1) - C(1,0) );
	  
	  return q;
	}*/
  
  
  
  int QuaternionLib::matrix_to_quat(const double rot[9], double quat[4])
  {
    quat[0] = 0.5*sqrt(rot[0]+rot[4]+rot[8]+1);

    if (fabs(quat[0]) > 1e-8) {
        double w4 = 1.0/(4.0*quat[0]);
      quat[1] = (rot[7]-rot[5]) * w4;
      quat[2] = (rot[2]-rot[6]) * w4;
      quat[3] = (rot[3]-rot[1]) * w4;
    }
    else {
      quat[1] = sqrt(fabs(-0.5*(rot[4]+rot[8])));
      quat[2] = sqrt(fabs(-0.5*(rot[0]+rot[8])));
      quat[3] = sqrt(fabs(-0.5*(rot[0]+rot[4])));
    }

    /*LSF: I may be missing something but this didn't work for me until I divided by the magnitude instead:
      double norm = quat[0]*quat[0] + quat[1]*quat[1] + quat[2]*quat[2] +
                       quat[3]*quat[3];
    if (fabs(norm) < 1e-10)
    return -1; */
    double norm = sqrt(quat[0]*quat[0] + quat[1]*quat[1] + quat[2]*quat[2] +
                       quat[3]*quat[3]);

    norm = 1/norm;
    quat[0] *= norm;
    quat[1] *= norm;
    quat[2] *= norm;
    quat[3] *= norm;

    return 0;
  }
  
  Eigen::Matrix<double, 3, 3> QuaternionLib::e2C(const Eigen::Vector3d &E) {
	  Eigen::Matrix<double, 3, 3> returnval;
	  
	  returnval.setIdentity();
	  
	  e2C(E,returnval);
	  
	  //std::cout << "QuaternionLib::e2C(const Eigen::Vector3d &E) -- SHOULD NOT BE USED YET" << std::endl;
	  
	  return returnval;
  }
  
  void QuaternionLib::e2C(Eigen::Vector3d const &Ec, Eigen::Matrix<double, 3, 3> &C)
  {
	  double st, sp, sps;
	  double ct, cp, cps;
	  
	  //std::cout << "e2C\n";

	  sp = sin(Ec(0));
	  st = sin(Ec(1));
	  sps = sin(Ec(2));
	  
	  cp = cos(Ec(0));
	  ct = cos(Ec(1));
	  cps = cos(Ec(2));
	  
	  C(0,0) = cps*ct;
	  C(0,1) = sps*ct;
	  C(0,2) = -st;
	  
	  C(1,0) = -sps*cp + cps*st*sp;
	  C(1,1) = cps*cp + sps * st * sp;
	  C(1,2) = ct*sp;
	  
	  C(2,0) = sps * sp + cps * st * cp;
	  C(2,1) = -cps*sp + sp * st * cp;
	  C(2,2) = ct * cp;
  }
  
  Eigen::Vector3d QuaternionLib::C2e(const Eigen::Matrix<double, 3, 3> &C__) {

	  Eigen::Quaterniond q__;
	  
	  q__ = C2q(C__);

	  q__.normalize();

	  //std::cout << "Q at this point is: " << q.w() << ", " << q.x() << ", " << q.y() << ", " << q.z() << std::endl;

	  return q2e(q__);
  }
      
  
  Eigen::Quaterniond QuaternionLib::e2q(const Eigen::Vector3d &E) {
	  //Eigen::Quaterniond q;
	  
	  std::cout << "e2q -- E = " << E.transpose() << std::endl;
	  
	  //return C2q(e2C(E));
	  
	  Eigen::Quaterniond q_return;
	  double roll = E(0), pitch = E(1), yaw = E(2);

     double halfroll = roll / 2.;
     double halfpitch = pitch / 2.;
     double halfyaw = yaw / 2.;

     double sin_r2 = sin (halfroll);
     double sin_p2 = sin (halfpitch);
     double sin_y2 = sin (halfyaw);

     double cos_r2 = cos (halfroll);
     double cos_p2 = cos (halfpitch);
     double cos_y2 = cos (halfyaw);

     q_return.w() = cos_r2 * cos_p2 * cos_y2 + sin_r2 * sin_p2 * sin_y2;
     q_return.x() = sin_r2 * cos_p2 * cos_y2 - cos_r2 * sin_p2 * sin_y2;
     q_return.y() = cos_r2 * sin_p2 * cos_y2 + sin_r2 * cos_p2 * sin_y2;
     q_return.z() = cos_r2 * cos_p2 * sin_y2 - sin_r2 * sin_p2 * cos_y2;
	  
     return q_return;
  }
  
  Eigen::Vector3d QuaternionLib::Cyaw_rotate(const Eigen::Matrix<double, 3, 3> &C, const Eigen::Vector3d &v) {
	  // This is a fairly inefficient implementation, but done like this to save time. This can be improved to be a direct conversion from C to E to C_yaw, rather than go through the quaternion conversion
	  
	  Eigen::Vector3d v_rot;
	  Eigen::Matrix<double, 3, 3> C_yaw;
	  Eigen::Vector3d E;
	  Eigen::Quaterniond q(C);
	  
	  E.setZero();
	  
	  q2e(q,E);
	  
	  E(0) = 0.;
	  E(1) = 0.;
	  
	  e2C(E,C_yaw); // TODO -- Check if we need to do a transpose here
	  
	  v_rot = C_yaw * v;
	  
	  return v_rot;
  }

  void QuaternionLib::printEulerAngles(std::string prefix, const Eigen::Isometry3d &isom) {
    Eigen::Quaterniond r(isom.linear());
    double ypr[3];
    quat_to_euler(r, ypr[0], ypr[1], ypr[2]);
    std::cout << prefix << " Euler Angles: " << ypr[0] << ", " << ypr[1] << ", " << ypr[2] << std::endl;
  }
  
  void QuaternionLib::printQuaternion(std::string preamble, const Eigen::Quaterniond &q) {
	  
	  std:: cout << preamble << " " << q.w() << ", " << q.x() << ", " << q.y() << ", " << q.z() << std::endl;
  }
  
  /* Not ready to be used yet, as the Eigen::Quaternion has this as a constructor -- but left here for when i need it later
  Eigen::Quaterniond QuaternionLib::C2q(const Eigen::Matrix<3,3,double> &C) const {
	Eigen::Quaterniond& q;
	
    float trace = a[0][0] + a[1][1] + a[2][2]; // I removed + 1.0f; see discussion with Ethan
    if( trace > 0 ) {// I changed M_EPSILON to 0
      float s = 0.5f / sqrtf(trace+ 1.0f);
      q.w() = 0.25f / s;
      q.x() = ( a[2][1] - a[1][2] ) * s;
      q.y() = ( a[0][2] - a[2][0] ) * s;
      q.z() = ( a[1][0] - a[0][1] ) * s;
    } else {
      if ( a[0][0] > a[1][1] && a[0][0] > a[2][2] ) {
        float s = 2.0f * sqrtf( 1.0f + a[0][0] - a[1][1] - a[2][2]);
        q.w() = (a[2][1] - a[1][2] ) / s;
        q.x() = 0.25f * s;
        q.y() = (a[0][1] + a[1][0] ) / s;
        q.z() = (a[0][2] + a[2][0] ) / s;
      } else if (a[1][1] > a[2][2]) {
        float s = 2.0f * sqrtf( 1.0f + a[1][1] - a[0][0] - a[2][2]);
        q.w() = (a[0][2] - a[2][0] ) / s;
        q.x() = (a[0][1] + a[1][0] ) / s;
        q.y() = 0.25f * s;
        q.z() = (a[1][2] + a[2][1] ) / s;
      } else {
        float s = 2.0f * sqrtf( 1.0f + a[2][2] - a[0][0] - a[1][1] );
        q.w() = (a[1][0] - a[0][1] ) / s;
        q.x() = (a[0][2] + a[2][0] ) / s;
        q.y() = (a[1][2] + a[2][1] ) / s;
        q.z() = 0.25f * s;
      }
    }
  }*/
}

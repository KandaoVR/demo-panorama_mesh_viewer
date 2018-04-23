/* Assorted utils based on OpenCV.
*  All rights reserved. KandaoVR 2017.
*  Contributor(s): Neil Z. Shao
*/
#pragma once
#include "opencv2/opencv.hpp"

namespace kandao { namespace opencv 
{
	static cv::RNG mRng = cv::RNG(0xFFFFFFFF);
	cv::Scalar randomColor(cv::RNG & rng = mRng);

	template <typename T>
	void makeVector(cv::Mat mat, std::vector<T> &values)
	{
		values.resize(mat.total());
		Mat _mat(mat.size(), mat.type(), values.data());
		mat.copyTo(_mat);
	}

	void subMatrix(const cv::Mat &frame, cv::Rect rec, cv::Mat &patch, cv::Scalar padding = 0);

	///////////////////////////////////// display /////////////////////////////////////
	cv::Mat viewableDepth(cv::Mat depth, int chns = 3);
	cv::Mat viewableDepth2Original(cv::Mat view_depth);
	cv::Mat viewableDisp(cv::Mat depth, float scale = 1., int type = CV_8UC3);
	cv::Mat viewableDisp2Original(cv::Mat view_disp, float scale = 1.);
	cv::Mat viewableByGradient(cv::Mat src, cv::Mat disp, float scale = 1./30);
	cv::Mat viewableFlow(cv::Mat flow);
	cv::Mat viewableFlow(cv::Mat flow_u, cv::Mat flow_v);
} }

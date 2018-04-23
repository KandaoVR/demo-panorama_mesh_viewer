/* Assorted utils based on OpenCV.
*  All rights reserved. KandaoVR 2017.
*  Contributor(s): Neil Z. Shao
*/
#include "utils/utils.opencv.h"

using namespace std;
using namespace cv;

namespace kandao { namespace opencv 
{
#define UNKNOWN_FLOW_THRESH 1e9  
#define MAX_DISPARITY 10000

	cv::Scalar randomColor(RNG &rng)
	{
		return CV_RGB(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
	}

	void subMatrix(const cv::Mat &frame, cv::Rect rec, cv::Mat &patch, cv::Scalar padding)
	{
		patch.create(rec.size(), frame.type());
		patch.setTo(padding);

		if (rec.x >= 0 && rec.y >= 0 && rec.br().x <= frame.cols && rec.br().y <= frame.rows) {
			frame(rec).copyTo(patch);
		} else {
			int sx0 = max(rec.x, 0), sy0 = max(rec.y, 0);
			int dx0 = sx0 - rec.x, dy0 = sy0 - rec.y;
			int sx1 = min(rec.br().x, frame.cols), sy1 = min(rec.br().y, frame.rows);
			int w = sx1 - sx0, h = sy1 - sy0;
			if (w > 0 && h > 0) {
				frame(Rect(sx0, sy0, w, h)).copyTo(patch(Rect(dx0, dy0, w, h)));
			}
		}
	}

	///////////////////////////////////// display /////////////////////////////////////
	cv::Mat viewableDepth(cv::Mat depth, int chns)
	{
		if (depth.depth() != CV_32F)
			depth.convertTo(depth, CV_32F);

		// y = 70 * (log(x + 100) - log(100))
		// x = exp(y / 70 + log(100)) - 100
		Mat view;
		log(depth + 100, view);
		view = (view - log(100)) * 70;
		view.setTo(0, view < 0);

		if (chns == 3) {
			cv::merge(vector<Mat>({ view, view, view }), view);
			view.convertTo(view, CV_8UC3);
		} else {
			view.convertTo(view, CV_8U);
		}

		return view;
	}

	cv::Mat viewableDepth2Original(cv::Mat view_disp)
	{
		if (view_disp.empty())
			return Mat();

		if (view_disp.channels() == 3) {
			vector<Mat> depths;
			split(view_disp, depths);
			view_disp = depths.front();
		}

		if (view_disp.depth() != CV_32F)
			view_disp.convertTo(view_disp, CV_32F);

		// y = 70 * (log(x + 100) - log(100))
		// x = exp(y / 70 + log(100)) - 100
		Mat depth;
		cv::exp(view_disp / 70. + log(100), depth);
		depth = depth - 100;
		depth.setTo(0, depth < 0);

		return depth;
	}

	cv::Mat viewableDisp(cv::Mat depth, float scale, int type)
	{
		if (depth.depth() != CV_32F)
			depth.convertTo(depth, CV_32F);

		Mat disp;
		divide(Scalar(25500 * scale), depth, disp);
		disp.setTo(0, depth <= 0);

		if (type == CV_8UC3) {
			cv::merge(vector<Mat>({ disp, disp, disp }), disp);
			disp.convertTo(disp, CV_8UC3);
		}
		else if (disp.type() != type) {
			disp.convertTo(disp, type);
		}

		return disp;
	}

	cv::Mat viewableDisp2Original(cv::Mat view_disp, float scale)
	{
		if (view_disp.empty())
			return Mat();

		if (view_disp.channels() == 3) {
			vector<Mat> disps;
			split(view_disp, disps);
			view_disp = disps.front();
		}

		if (view_disp.depth() != CV_32F)
			view_disp.convertTo(view_disp, CV_32F);

		Mat depth;
		divide(Scalar(25500 * scale), view_disp, depth);
		depth.setTo(0, depth <= 0);
		depth.setTo(MAX_DISPARITY, view_disp <= 0);

		return depth;
	}

	cv::Mat viewableByGradient(cv::Mat src, cv::Mat disp, float scale)
	{
		Mat gx, gy;
		cv::Mat kernelx = (cv::Mat_<float>(1, 2) << -1, 1);
		cv::Mat kernely = (cv::Mat_<float>(2, 1) << 1, -1);
		cv::filter2D(src, gx, CV_32F, kernelx, cv::Point(0, 0), 0, cv::BORDER_REPLICATE);
		cv::filter2D(src, gy, CV_32F, kernely, cv::Point(0, 1), 0, cv::BORDER_REPLICATE);

		Mat grad;
		cv::multiply(gx, gx, gx);
		cv::multiply(gy, gy, gy);
		cv::add(gx, gy, grad);
		cv::transform(grad, grad, cv::Matx13f(1, 1, 1));
		cv::sqrt(grad, grad);

		Mat weight;
		cv::multiply(grad, scale, weight);
		weight.setTo(1, weight > 1);
		cv::pow(weight, 5, weight);
		if (weight.channels() == 1)
			cv::cvtColor(weight, weight, CV_GRAY2BGR);

		Mat disp_grad;
		applyColorMap(disp, disp_grad, COLORMAP_JET);
		disp_grad.convertTo(disp_grad, CV_32FC3);
		cv::multiply(disp_grad, weight, disp_grad);
		disp_grad.convertTo(disp_grad, CV_8UC3);

		return disp_grad;
	}

	void makecolorwheel(vector<Scalar> &colorwheel)
	{
		int RY = 15;
		int YG = 6;
		int GC = 4;
		int CB = 11;
		int BM = 13;
		int MR = 6;

		int i;

		for (i = 0; i < RY; i++) colorwheel.push_back(Scalar(255, 255 * i / RY, 0));
		for (i = 0; i < YG; i++) colorwheel.push_back(Scalar(255 - 255 * i / YG, 255, 0));
		for (i = 0; i < GC; i++) colorwheel.push_back(Scalar(0, 255, 255 * i / GC));
		for (i = 0; i < CB; i++) colorwheel.push_back(Scalar(0, 255 - 255 * i / CB, 255));
		for (i = 0; i < BM; i++) colorwheel.push_back(Scalar(255 * i / BM, 0, 255));
		for (i = 0; i < MR; i++) colorwheel.push_back(Scalar(255, 0, 255 - 255 * i / MR));
	}

	void motionToColor(Mat flow, Mat &color)
	{
		if (color.empty())
			color.create(flow.rows, flow.cols, CV_8UC3);

		static vector<Scalar> colorwheel; //Scalar r,g,b  
		if (colorwheel.empty())
			makecolorwheel(colorwheel);

		// determine motion range:  
		//	float maxrad = -1;
		float maxrad = 64 + 2.2204e-16;
		// Find max flow to normalize fx and fy  
#if 0
		for (int i = 0; i < flow.rows; ++i)
		{
			for (int j = 0; j < flow.cols; ++j)
			{
				Vec2f flow_at_point = flow.ptr<Vec2f>(i)[j];
				float fx = flow_at_point[0];
				float fy = flow_at_point[1];
				if ((fabs(fx) > UNKNOWN_FLOW_THRESH) || (fabs(fy) > UNKNOWN_FLOW_THRESH))
					continue;
				//			float rad = sqrt(fx * fx + fy * fy);
				//			maxrad = maxrad > rad ? maxrad : rad;
			}
		}
#endif
		for (int i = 0; i < flow.rows; ++i)
		{
			for (int j = 0; j < flow.cols; ++j)
			{
				uchar *data = color.data + color.step[0] * i + color.step[1] * j;
				float fx, fy;
				if (flow.type() == CV_32FC2) {
					Vec2f flow_at_point = flow.ptr<Vec2f>(i)[j];
					fx = flow_at_point[0] / maxrad;
					fy = flow_at_point[1] / maxrad;
				}
				else if (flow.type() == CV_16SC2) {
					Vec2s flow_at_point = flow.ptr<Vec2s>(i)[j];
					fx = flow_at_point[0] / maxrad;
					fy = flow_at_point[1] / maxrad;
				}
				else {
					fx = 0;
					fy = 0;
				}

				if ((fabs(fx) > UNKNOWN_FLOW_THRESH) || (fabs(fy) > UNKNOWN_FLOW_THRESH))
				{
					data[0] = data[1] = data[2] = 0;
					continue;
				}
				float rad = sqrt(fx * fx + fy * fy);

				float angle = atan2(-fy, -fx) / CV_PI;
				float fk = (angle + 1.0) / 2.0 * (colorwheel.size() - 1) + 1;
				int k0 = (int)fk;
				int k1 = (k0 + 1) % colorwheel.size();
				float f = fk - k0;
				//f = 0; // uncomment to see original color wheel  

				for (int b = 0; b < 3; b++)
				{
					float col0 = colorwheel[k0][b] / 255.0;
					float col1 = colorwheel[k1][b] / 255.0;
					float col = (1 - f) * col0 + f * col1;
					if (rad <= 1)
						col = 1 - rad * (1 - col); // increase saturation with radius  
					else
						col *= .75; // out of range  
					data[2 - b] = (int)(255.0 * col);
				}
			}
		}
	}

	cv::Mat viewableFlow(cv::Mat flow)
	{
		Mat view_flow;
		motionToColor(flow, view_flow);
		return view_flow;
	}

	cv::Mat viewableFlow(cv::Mat flow_u, cv::Mat flow_v)
	{
		vector<Mat> flows = { flow_u, flow_v };
		Mat flow;
		merge(flows, flow);
		Mat view_flow;
		motionToColor(flow, view_flow);
		return view_flow;
	}
} }

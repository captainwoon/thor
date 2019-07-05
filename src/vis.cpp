//
// Created by jintian on 18-1-12.
//

#include "include/vis.h"

cv::Scalar thor::vis::gen_unique_color_cv(int idx, bool is_track, double hue_step, float alpha) {
  RGBA cr = gen_unique_color(idx, is_track, hue_step, alpha);
  cv::Scalar c(cr.r, cr.g, cr.b, cr.a);
  return c;
}

thor::vis::RGBA thor::vis::gen_unique_color(int idx, bool is_track, double hue_step,
                                            float alpha) {

  // if idx is track id, the color should be 
  if (is_track) {
    // we may have 1000+ track ids
    int track_size = 1./hue_step;
    idx = idx%track_size;
  }
  auto h = int(idx * (360 * hue_step));
  // 1/5 values we will not use
  double v = 1.0 - ((idx * 1.0) * hue_step) / 5.;
  float s = 1;
  float r, g, b;
  thor::vis::hsv2rgb(r, g, b, h, s, v);

  thor::vis::RGBA rgba;
  rgba.r = int(255 * r);
  rgba.g = int(255 * g);
  rgba.b = int(255 * b);
  rgba.a = int(alpha * 255);
  return rgba;
}

void thor::vis::hsv2rgb(float &r, float &g, float &b, int h, float s,
                        double v) {
  double hh, p, q, t, ff;
  long i;
  if (s <= 0.0) {  // < is bogus, just shuts up warnings
    r = float(v);
    g = float(v);
    b = float(v);
  }
  hh = h;
  if (hh >= 360.0) hh = 0.0;
  hh /= 60.0;
  i = (long)hh;
  ff = hh - i;
  p = v * (1.0 - s);
  q = v * (1.0 - (s * ff));
  t = v * (1.0 - (s * (1.0 - ff)));
  switch (i) {
    case 0:
      r = v;
      g = t;
      b = p;
      break;
    case 1:
      r = q;
      g = v;
      b = p;
      break;
    case 2:
      r = p;
      g = v;
      b = t;
      break;

    case 3:
      r = p;
      g = q;
      b = v;
      break;
    case 4:
      r = t;
      g = p;
      b = v;
      break;
    case 5:
    default:
      r = v;
      g = p;
      b = q;
      break;
  }
}

cv::Mat thor::vis::createAlpha(cv::Mat &src) {
  cv::Mat alpha = cv::Mat::zeros(src.rows, src.cols, CV_8UC1);
  cv::Mat gray = cv::Mat::zeros(src.rows, src.cols, CV_8UC1);
  cv::cvtColor(src, gray, cv::COLOR_RGB2GRAY);
  for (int i = 0; i < src.rows; i++) {
    for (int j = 0; j < src.cols; j++) {
      alpha.at<uchar>(i, j) = 255 - gray.at<uchar>(i, j);
    }
  }
  return alpha;
}

int thor::vis::addAlpha(cv::Mat &src, cv::Mat &dst, cv::Mat &alpha) {
  if (src.channels() == 4) {
    return -1;
  } else if (src.channels() == 1) {
    cv::cvtColor(src, src, cv::COLOR_GRAY2RGB);
  }

  dst = cv::Mat(src.rows, src.cols, CV_8UC4);

  std::vector<cv::Mat> srcChannels;
  std::vector<cv::Mat> dstChannels;
  cv::split(src, srcChannels);

  dstChannels.push_back(srcChannels[0]);
  dstChannels.push_back(srcChannels[1]);
  dstChannels.push_back(srcChannels[2]);
  dstChannels.push_back(alpha);
  cv::merge(dstChannels, dst);
  return 0;
}


namespace thor{
namespace vis{


/**
 * Human pose order should in OpenPose order
 *
 * @param poses
 * @param image
 */
void renderHumanPose(std::vector<HumanPose>& poses, cv::Mat& image) {
  // drawing HumanPoses on image
  CV_Assert(image.type() == CV_8UC3);
  const std::vector<cv::Scalar> colors = {
      cv::Scalar(255, 0, 0), cv::Scalar(255, 85, 0), cv::Scalar(255, 170, 0),
      cv::Scalar(255, 255, 0), cv::Scalar(170, 255, 0), cv::Scalar(85, 255, 0),
      cv::Scalar(0, 255, 0), cv::Scalar(0, 255, 85), cv::Scalar(0, 255, 170),
      cv::Scalar(0, 255, 255), cv::Scalar(0, 170, 255), cv::Scalar(0, 85, 255),
      cv::Scalar(0, 0, 255), cv::Scalar(85, 0, 255), cv::Scalar(170, 0, 255),
      cv::Scalar(255, 0, 255), cv::Scalar(255, 0, 170), cv::Scalar(255, 0, 85)
  };
  const std::vector<std::pair<int, int> > limbKeypointsIds = {
      {1, 2},  {1, 5},   {2, 3},
      {3, 4},  {5, 6},   {6, 7},
      {1, 8},  {8, 9},   {9, 10},
      {1, 11}, {11, 12}, {12, 13},
      {1, 0},  {0, 14},  {14, 16},
      {0, 15}, {15, 17}
  };

  const int stickWidth = 3;
  const cv::Point2f absentKeypoint(-1.0f, -1.0f);
  for (const auto& pose : poses) {
    // we only support 18 keypoints
    CV_Assert(pose.keypoints.size() == 18);

    for (size_t keypointIdx = 0; keypointIdx < pose.keypoints.size(); keypointIdx++) {
      if (pose.keypoints[keypointIdx] != absentKeypoint) {
        cv::circle(image, pose.keypoints[keypointIdx], 3, colors[keypointIdx], -1);
      }
    }
  }
  cv::Mat pane = image.clone();
  for (auto &pose : poses) {
    for (const auto& limbKeypointsId : limbKeypointsIds) {
      std::pair<cv::Point2f, cv::Point2f> limbKeypoints(pose.keypoints[limbKeypointsId.first],
                                                        pose.keypoints[limbKeypointsId.second]);
      if (limbKeypoints.first == absentKeypoint
          || limbKeypoints.second == absentKeypoint) {
        continue;
      }

      float meanX = (limbKeypoints.first.x + limbKeypoints.second.x) / 2;
      float meanY = (limbKeypoints.first.y + limbKeypoints.second.y) / 2;
      cv::Point difference = limbKeypoints.first - limbKeypoints.second;
      double length = std::sqrt(difference.x * difference.x + difference.y * difference.y);
      int angle = static_cast<int>(std::atan2(difference.y, difference.x) * 180 / CV_PI);
      std::vector<cv::Point> polygon;
      cv::ellipse2Poly(cv::Point2d(meanX, meanY), cv::Size2d(length / 2, stickWidth),
                       angle, 0, 360, 1, polygon);
      cv::fillConvexPoly(pane, polygon, colors[limbKeypointsId.second]);
    }
    // for every pose, if pose has pose_id, means it is tracked
    if(pose.pose_id != -1) {
      // we draw this id
      Box b = pose.to_box();
      cv::putText(image, to_string(pose.pose_id),
          Point2f(b.xmin, b.ymin), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 0, 255));
      cv::rectangle(image, Point2f(b.xmin, b.ymin), Point2f(b.xmax, b.ymax),
          Scalar(255, 0, 0), 1);
    }
  }
  cv::addWeighted(image, 0.6, pane, 0.7, 0, image);
}

void renderHumanPoseSimple(std::vector<HumanPose>& poses, cv::Mat& image) {
  // a more simple render of human pose estimation
  // we can define our own limbKeypoints to unlink some joint
  const std::vector<std::pair<int, int> > limbKeypointsIds = {
      {1, 2},  {1, 5},   {2, 3},
      {3, 4},  {5, 6},   {6, 7},
      {1, 8},  {8, 9},   {9, 10},
      {1, 11}, {11, 12}, {12, 13},
      {1, 0},  {0, 14},  {14, 16},
      {0, 15}, {15, 17}
  };
  const cv::Point2f absentKeypoint(-1.0f, -1.0f);
  for (auto& pose : poses) {
    for (const auto& limbKeypointsId : limbKeypointsIds) {
      std::pair<cv::Point2f, cv::Point2f> limbKeypoints(pose.keypoints[limbKeypointsId.first],
                                                        pose.keypoints[limbKeypointsId.second]);
      if (limbKeypoints.first == absentKeypoint
          || limbKeypoints.second == absentKeypoint) {
        continue;
      }
      cv::line(image, limbKeypoints.first, limbKeypoints.second, Scalar(255, 255, 255), 1);
    }
    if(pose.pose_id != -1) {
      // we draw this id
      Box b = pose.to_box();
      cv::putText(image, to_string(pose.pose_id),
                  Point2f(b.xmin, b.ymin), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));
	  cv::rectangle(image, Point2f(b.xmin, b.ymin), Point2f(b.xmax, b.ymax),
					Scalar(255, 255, 255), 1);
    }
  }
}


}
}
#pragma once

#include "matplotlibcpp.h"

#include <mutex>
#include <thread>

namespace plt = matplotlibcpp;
class Viewer {
  public:
    Viewer() {
      shouldQuit_ = false;
    }
    ~Viewer() {
      quit();
    }

    void quit() {
      shouldQuit_ = true;
    }

    void InitAsCallBack(std::map<int, Robot> iniRobots, 
                std::map<int, Eigen::Vector2d> anchorPositions) {
      std::unique_lock<std::mutex> lk(mutex_);

      nRobot_ = iniRobots.size();
      nAnchor_ = anchorPositions.size();
      // TODO: nRobot_ may be 1
      arrowX_.reserve(nRobot_);
      arrowY_.reserve(nRobot_);
      arrowU_.reserve(nRobot_);
      arrowV_.reserve(nRobot_);
      ancPositions_ = anchorPositions;
    }

    void UpdateAsCallBack(std::vector<PoseResults> res) {
      std::unique_lock<std::mutex> lk(mutex_);
      arrowX_.clear();
      arrowY_.clear();
      arrowU_.clear();
      arrowV_.clear();
      for (auto&r : res) {
        int rId = r.id;
        if (x_[rId].size() >= 1e6) {
          x_[rId].erase(x_[rId].begin());
          y_[rId].erase(y_[rId].begin());
          yaw_[rId].erase(yaw_[rId].begin());
        }
        x_[rId].push_back(r.x);
        y_[rId].push_back(r.y);
        yaw_[rId].push_back(r.yaw);

        // Orientation arrow
        // u and v are respectively the x and y components of the arrows we're plotting
        arrowX_.push_back(r.x);
        arrowY_.push_back(r.y);
        double u = cos(r.yaw) * 0.2;
        double v = sin(r.yaw) * 0.2;
        arrowU_.push_back(u);
        arrowV_.push_back(v);
      }
    }

    void PlottingLoop() {
      plt::title("Collaborative Localization");
      plt::axis("equal");

      while (1) {
        if (shouldQuit_)
          return;
        // Clear previous plot
        plt::clf();

        std::unique_lock<std::mutex> lk(mutex_);

        // Plot anchors
        for (auto& anc : ancPositions_) {
          plt::plot(std::vector<double>{anc.second(0)}, std::vector<double>{anc.second(1)});
          plt::text(anc.second(0), anc.second(1), "Anchor " + std::to_string(anc.first));
        }

        for (auto& r: x_) {
          int rId = r.first;
          // Plot robot trajectories
          plt::plot(x_[rId], y_[rId]);
          plt::text(x_[rId].back(), y_[rId].back(), "Robot " + std::to_string(rId));
        }

        ResizeWindow();

        // plot arrows
        plt::quiver(arrowX_, arrowY_, arrowU_, arrowV_);

        lk.unlock();

        plt::pause(0.01);
      }
    }

    void ResizeWindow() {
      double xMin = 1e9, xMax = -1e9, yMin = 1e9, yMax = -1e9;
      for (auto& xItem : x_) {
        for (auto& x : xItem.second) {
          if (x < xMin)
            xMin = x;
          if (x > xMax)
            xMax = x;
        }
      }
      for (auto& yItem : y_) {
        for (auto& y : yItem.second) {
          if (y < yMin) 
            yMin = y;
          if (y > yMax)
            yMax = y;
        }
      }
      for (auto& ancItem : ancPositions_) {
        double ancX = ancItem.second(0);
        double ancY = ancItem.second(1);
        if (ancX < xMin)
          xMin = ancX;
        if (ancX > xMax)
          xMax = ancX;
        if (ancY < yMin)
          yMin = ancY;
        if (ancY > yMax)
          yMax = ancY;
      }
      plt::xlim(xMin - 0.5, xMax + 0.5);
      plt::ylim(yMin - 0.2, yMax + 0.2);
    }

  private:
    int nRobot_;
    int nAnchor_;
    std::map<int, std::vector<double>> x_;
    std::map<int, std::vector<double>> y_;
    std::map<int, std::vector<double>> yaw_;
    std::map<int, Eigen::Vector2d> ancPositions_;
    std::vector<double> arrowX_;   // arrow start point
    std::vector<double> arrowY_;
    std::vector<double> arrowU_;   // arrow end point
    std::vector<double> arrowV_;

    std::mutex mutex_;

    std::atomic_bool shouldQuit_;
};
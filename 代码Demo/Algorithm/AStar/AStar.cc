#include <queue>
#include <vector>
#include <cstdint>
#include <cmath>
#include <array>
#include <iostream>

class AStar
{
  private:
    struct Point
    {
        int32_t x_;
        int32_t y_;
        double expect_;
    };

    struct PointComparer
    {
        bool operator()(const Point& lhs, const Point& rhs)
        {
            return lhs.expect_ < rhs.expect_;
        }
    };

    std::vector<std::vector<char>> graph_;
    std::vector<std::vector<bool>> travelNote_;
    std::vector<std::vector<Point>> trace_;
    std::vector<std::vector<double>> forecaseScore_;
    std::vector<std::vector<double>> actualScore_;
    std::priority_queue<Point, std::vector<Point>, PointComparer> heap_;

   

    double estimate(int64_t lx, int64_t ly, int64_t rx, int64_t ry)
    {
        return sqrt((lx - rx) * (lx - rx) + (ly - ry) * (ly - ry));
    }

    bool validIndex(int32_t x, int32_t y)
    {
        return !graph_.empty() && 
               x >= 0 && x < graph_.size() && 
               y >= 0 && y < graph_[0].size() &&
               graph_[x][y] != WALL;
    }

  public:

    static constexpr char WALL = 0xff;
    static constexpr double DIAGONAL = 1.4142135623730;
    static const std::array<std::array<int, 2>, 8> direction;

    AStar(const std::vector<std::vector<char>>& graph)
      : graph_(graph)
    {
        if(graph.empty())
          return;
        trace_ = std::vector<std::vector<Point>>(graph.size(), 
                                                 std::vector<Point>(graph[0].size(), {-1, -1, 0}));
        travelNote_ = std::vector<std::vector<bool>>(graph.size(), 
                                                     std::vector<bool>(graph[0].size(), false));
        forecaseScore_ = std::vector<std::vector<double>>(graph.size(), 
                                                          std::vector<double>(graph[0].size(), 0.0));
        actualScore_ = std::vector<std::vector<double>>(graph.size(), 
                                                        std::vector<double>(graph[0].size(), 0.0));
    };

    void run(int32_t xb, int32_t yb, int32_t xd, int32_t yd)
    {
        if(graph_[xb][yb] == WALL)
            return;
        travelNote_[xb][yb] = true;
        forecaseScore_[xb][yb] = actualScore_[xb][yb] + estimate(xb, yb, xd, yd);
        heap_.push({xb, yb, forecaseScore_[xb][yb]});
        while(!heap_.empty())
        {
            auto curPoint = heap_.top();
            heap_.pop();
            int cx{curPoint.x_}, cy{curPoint.y_};
            if(cx == xd && cy == yd)
            {
                uint32_t x = xd, y = yd;
                while(!(x == xb && y == yb))
                {
                    std::cout << x << "  " << y << std::endl;
                    uint32_t tx = trace_[x][y].x_;
                    uint32_t ty = trace_[x][y].y_;
                    x = tx;
                    y = ty;
                }
                std::cout << x << "  " << y << std::endl;
                return;     // found minipath
            }
            for(int dirIndex = 0; dirIndex < direction.size(); ++dirIndex)
            {
                int32_t nx{cx + direction[dirIndex][0]}, ny{cy + direction[dirIndex][1]};
                if(!validIndex(nx, ny))
                    continue;
                double forecase{0.0}, actual{0.0};
                if(direction[dirIndex][0] & direction[dirIndex][1])
                {
                    forecase += 1;
                }
                else
                {
                    forecase += DIAGONAL;
                }
                actual = forecase + actualScore_[cx][cy];
                forecase += actual + estimate(nx, ny, xd, yd);

                if(travelNote_[nx][ny] == false || actual < actualScore_[nx][ny])
                {
                    trace_[nx][ny] = {cx, cy, forecase};
                    forecaseScore_[nx][ny] = forecase;
                    actualScore_[nx][ny] = actual;
                    if(!travelNote_[nx][ny])
                    {
                        travelNote_[nx][ny] = true;
                        heap_.push({nx, ny, forecase});
                    }
                }
            }
        }
        return;
    }
};

constexpr std::array<std::array<int, 2>, 8> AStar::direction = 
    {{{-1, 0}, {1, 0}, {0, -1}, {0, 1}, 
      {-1, -1}, {-1, 1}, {1, -1}, {1, 1}}};

int main(int argc, char const *argv[])
{
    char WALL = 0xff;
    std::vector<std::vector<char>> graph = 
        {{1, 0, WALL, 0, 1,    1, 0},
         {1, 0, WALL, 1, 0,    0, 1},
         {1, 0, WALL, 1, 0,    0, 1},
         {1, 0, WALL, 1, WALL, 0, 1},
         {0, 1, 1,    0, WALL, 0, 1}};

    AStar astar(graph);
    astar.run(0, 0, 4, 6);
    return 0;
}

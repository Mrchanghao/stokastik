import collections

class Solution(object):
    def get_max_path_one(self, grid, p1, p2, cached):

        x = p1[0] < len(grid) and p1[1] < len(grid) and grid[p1[0]][p1[1]] != -1
        y = p2[0] < len(grid) and p2[1] < len(grid) and grid[p2[0]][p2[1]] != -1

        if x and y:
            if (p1, p2) in cached:
                w = cached[(p1, p2)]
            else:
                w = self.get_max_path(grid, p1, p2, cached)

        else:
            w = -float("Inf")

        return w


    def get_max_path(self, grid, point_1, point_2, cached):

        if point_1 == point_2 == (len(grid) - 1, len(grid) - 1):
            cached[(point_1, point_2)] = grid[len(grid) - 1][len(grid) - 1]

        else:
            p1, q1 = (point_1[0] + 1, point_1[1]), (point_1[0], point_1[1] + 1)
            p2, q2 = (point_2[0] + 1, point_2[1]), (point_2[0], point_2[1] + 1)

            a = self.get_max_path_one(grid, p1, p2, cached)
            b = self.get_max_path_one(grid, q1, q2, cached)
            c = self.get_max_path_one(grid, p1, q2, cached)

            if point_1 == point_2:
                x = grid[point_1[0]][point_1[1]]
                cached[(point_1, point_2)] = x + max(a, b, c)

            else:
                x = grid[point_1[0]][point_1[1]] + grid[point_2[0]][point_2[1]]
                d = self.get_max_path_one(grid, q1, p2, cached)

                cached[(point_1, point_2)] = x + max(a, b, c, d)

        return cached[(point_1, point_2)]


    def cherryPickup(self, grid):
        cached = collections.defaultdict(int)

        out = self.get_max_path(grid, (0,0), (0,0), cached)

        if out == -float("Inf"):
            return 0

        return out

sol = Solution()
print sol.cherryPickup([[1,-1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,1,-1,1,1,1,1,-1,1,1,1,1,-1,1,1,1,1,1,-1,1,1,1,-1,1,1,1,1,1,1,1,1],[1,1,-1,1,-1,-1,-1,-1,1,1,-1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,-1,1,1,1,1,1,-1,1,-1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1],[1,1,1,1,1,1,1,-1,1,1,1,1,-1,1,-1,1,-1,1,1,1,1,1,1,1,1,-1,1,1,1,-1,-1,-1,1,-1,1,1,1,1,-1,1,-1,-1,-1,1,1,-1,-1,1,1,1],[1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,-1,1,1,1,1,1,1,1,1,1,1,-1,1,1,-1],[1,1,1,1,1,1,1,1,1,1,1,1,-1,-1,1,1,1,-1,1,1,1,-1,-1,-1,1,1,1,1,1,-1,-1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,1],[-1,1,-1,1,-1,1,-1,1,1,1,1,1,1,-1,1,1,1,1,-1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,-1,1,1,1,1,1,1,-1],[1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,-1,1,1,1,1,-1,1,-1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,-1,1,1],[1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,1,-1,1,1,1,1,1,1,1,1,-1,1,1,1,1,-1,1,1,1,-1,1,-1,1,1,1,1,-1],[1,1,1,1,1,-1,1,1,1,1,1,-1,-1,1,1,1,1,-1,1,-1,1,1,-1,1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,-1,1,1,1],[1,1,1,1,1,1,-1,1,1,-1,1,-1,1,1,1,1,1,-1,1,1,1,1,1,1,-1,1,1,1,1,-1,1,1,1,-1,1,1,1,1,1,-1,1,-1,-1,1,1,1,1,1,-1,1],[-1,1,1,1,1,1,1,1,1,-1,1,-1,1,1,1,1,1,1,1,1,1,-1,1,1,1,-1,1,-1,-1,1,1,-1,1,1,-1,1,1,1,1,1,1,1,-1,-1,1,-1,1,1,1,1],[1,1,1,-1,1,1,1,-1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,-1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,-1,1],[1,-1,1,1,1,1,1,-1,1,1,-1,-1,1,1,1,1,1,1,-1,1,1,1,1,-1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,1,-1,1,1,-1,1,1,1,1,-1],[1,-1,1,1,-1,1,1,-1,1,1,1,-1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1,-1,1],[1,1,1,1,1,-1,1,-1,1,1,1,-1,1,1,1,1,1,1,-1,1,1,-1,-1,1,1,1,1,1,1,1,-1,1,1,-1,1,1,1,1,-1,1,-1,1,1,1,1,1,-1,-1,1,-1],[1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,-1,1,1,-1,1,1,1,1,1,1,1,1,1,-1,-1,1,1,1,-1,1,1,1,-1,1,1,1,1],[1,1,1,1,-1,1,1,1,1,1,1,1,1,-1,1,-1,1,1,-1,1,1,1,1,-1,1,1,1,1,-1,1,1,1,-1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,-1,1],[1,-1,1,1,1,1,1,1,1,1,1,-1,1,1,1,0,1,1,1,-1,1,1,-1,1,1,1,1,-1,1,-1,1,1,1,1,1,-1,1,1,-1,1,1,-1,1,-1,1,1,1,1,1,-1],[1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,-1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,-1,1,-1,1,1,1,1,1,1,1,1,1],[1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,-1,1,1,1,1,-1,1,-1,1,1,1,1,1,1,-1,1,1,-1,-1,1,1,-1,1,-1,1],[1,-1,-1,-1,1,-1,1,1,1,1,1,1,1,-1,-1,1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,-1,1,1,1,1,-1,1,1,1,1,-1,1,-1,-1,-1,1,1,1,1],[-1,-1,-1,1,-1,1,1,1,-1,1,1,1,-1,-1,1,1,1,-1,1,-1,1,-1,1,-1,1,1,1,1,1,1,1,-1,-1,1,1,-1,-1,1,1,1,1,-1,1,1,1,-1,-1,1,1,1],[1,1,1,1,1,-1,1,1,-1,-1,1,1,1,1,-1,1,1,1,1,1,-1,-1,1,1,-1,1,1,-1,1,1,1,-1,1,1,-1,1,1,1,1,1,1,-1,1,1,-1,1,1,1,1,1],[1,-1,1,-1,-1,1,1,1,1,1,1,1,-1,1,1,1,-1,1,1,1,1,1,1,-1,1,1,1,1,-1,1,1,1,1,1,1,1,-1,1,1,-1,1,1,1,-1,1,-1,1,-1,1,-1],[1,1,1,1,-1,1,1,1,1,1,1,1,0,1,-1,1,1,-1,1,1,1,1,1,1,1,-1,1,1,1,1,-1,-1,-1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,1],[1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1,1,-1,1,1,1,-1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,-1,1,1,1,1,1,-1,1,1,1,1,-1,1,1,1],[1,1,-1,1,1,1,1,1,-1,1,1,1,1,1,1,-1,1,1,1,-1,-1,1,1,1,-1,1,1,1,1,1,-1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1,1,1,1,1],[-1,-1,1,1,1,1,1,-1,1,1,-1,1,1,1,1,1,1,1,1,-1,-1,1,1,1,-1,1,1,1,1,1,1,-1,1,-1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1],[1,1,-1,1,-1,1,-1,1,-1,1,1,1,1,1,-1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,1],[1,1,1,1,1,-1,1,-1,1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,-1,1,1,-1,1,1,-1,1,1,1,1,1,1,1,1,1,1,1,-1],[1,-1,1,1,-1,-1,1,1,1,-1,-1,1,1,1,-1,-1,1,1,1,-1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1,-1,1,1,1,1,1,-1,-1,1,1,-1,1,1],[1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,1,-1,1,1,1,-1,1,1,1,1,1,1,1,1,-1,1,1,-1,-1,-1,1,1,1,-1,-1,1,-1,1],[1,1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,-1,1,1,1,1,1,-1,1,1,1,1,-1,1,-1,1,1,1,-1,-1,1,1,1,1,-1,1,1,1,1,1,1,-1,1,1],[1,-1,-1,1,1,-1,1,1,1,1,-1,-1,1,-1,1,-1,1,1,1,-1,1,1,1,1,1,1,1,1,1,-1,1,1,1,-1,1,1,-1,1,1,1,1,1,1,1,1,-1,1,1,1,-1],[-1,1,-1,1,1,-1,1,1,1,1,1,1,-1,1,1,1,1,-1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1,-1,1,-1,1,-1,-1,1,1,1,-1,1,1,1,1,-1,-1,1,-1],[1,1,1,1,1,1,1,1,1,1,1,-1,1,-1,1,1,-1,1,-1,1,-1,1,1,1,1,1,-1,1,1,1,1,1,-1,1,-1,1,1,1,1,1,1,1,1,1,-1,1,-1,1,1,-1],[1,1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1,-1,1,1,1,1,-1,1,1,1,-1,1,1,-1,-1,1,1,1,1,1,1,1,1,-1,1,1,1,1,-1,1,1,1,1,1],[1,1,-1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,-1,1,1,1,1,-1,1,1,-1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1,1,1,-1,1,1,1],[1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,-1,-1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,-1,1,1,1],[1,1,-1,-1,1,1,-1,1,-1,1,1,1,1,1,-1,1,-1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1],[1,1,1,1,1,-1,1,1,-1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1,1,-1,1,1,1,-1,-1,1,1,1,1,-1,1,1,-1,-1,-1,1,1,1,-1,1],[1,1,1,1,1,-1,-1,1,1,1,1,1,1,-1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1,-1,1,1,1,1,1,-1,1,1,1,1,-1,1,1,1,1,1,1,1,1,1,1],[1,1,1,1,1,-1,1,1,1,1,-1,-1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,-1,1,1,1,1],[1,1,1,1,1,1,-1,1,1,1,-1,1,1,1,1,1,-1,1,1,1,1,1,-1,1,1,1,1,1,1,-1,1,1,1,1,1,1,-1,1,1,1,1,1,-1,1,1,-1,1,1,1,1],[1,1,-1,1,1,1,1,1,0,1,1,1,-1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1,-1,1,1,1,1,1,-1,1,1,1,1,1,1],[1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,-1,1,1,1,-1,1,1,1,1,1,1,-1,1,1,-1,1,1,-1,1,1,1,1,1,1,1,1,1,-1,1,1,-1,1,1,1,1],[1,1,-1,1,1,1,1,1,1,1,1,1,1,-1,1,1,-1,1,1,1,-1,1,-1,1,1,-1,1,-1,1,-1,1,1,1,1,1,1,1,1,1,1,-1,1,-1,1,1,1,1,1,-1,-1],[1,1,1,-1,1,1,1,1,1,-1,-1,-1,1,-1,1,1,-1,-1,1,1,1,1,-1,-1,-1,1,1,1,-1,1,1,1,1,1,1,1,-1,-1,1,-1,1,1,1,1,1,-1,1,-1,1,1],[1,1,-1,-1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,1,1,1,1,1,1,-1,1,1,1,1,1,-1,-1,1,1,-1,1,1,1,-1,1,-1,1,1,1,-1,1,1,1,1,1],[1,1,1,-1,-1,1,1,-1,1,-1,1,1,1,-1,1,1,-1,-1,-1,1,1,1,1,1,1,1,-1,1,1,1,1,-1,1,1,-1,1,1,1,1,1,1,1,-1,1,1,1,-1,1,1,1]])
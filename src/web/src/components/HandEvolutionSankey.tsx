import React, { useMemo } from "react";
import { sankey, sankeyLinkHorizontal, SankeyNode, SankeyLink } from "d3-sankey";
import { Card, CardContent, CardHeader, CardTitle } from "./ui/card";
import { EvolutionPathData } from "../api/contract";

interface HandEvolutionSankeyProps {
  handName: string;
  evolutionPaths: EvolutionPathData[];
}

interface CustomSankeyNode extends SankeyNode<{}, {}> {
  name: string;
  stage: number;
}

interface CustomSankeyLink extends SankeyLink<CustomSankeyNode, {}> {
  value: number;
}

export const HandEvolutionSankey: React.FC<HandEvolutionSankeyProps> = ({
  handName,
  evolutionPaths,
}) => {
  const { nodes, links } = useMemo(() => {
    // Build nodes and links from evolution paths
    const nodeMap = new Map<string, number>();
    const nodeArray: { name: string; stage: number }[] = [];
    const linkArray: { source: number; target: number; value: number }[] = [];

    const getNodeId = (stateName: string, stage: number): number => {
      const key = `${stateName}_${stage}`;
      if (!nodeMap.has(key)) {
        const id = nodeArray.length;
        nodeMap.set(key, id);
        nodeArray.push({ name: stateName, stage });
      }
      return nodeMap.get(key)!;
    };

    // Process each path
    for (const { pathString, count } of evolutionPaths) {
      if (!pathString) continue;
      const states = pathString.split("->");

      for (let i = 0; i < states.length - 1; i++) {
        const sourceId = getNodeId(states[i], i);
        const targetId = getNodeId(states[i + 1], i + 1);

        // Find or create link
        const existingLink = linkArray.find(
          (l) => l.source === sourceId && l.target === targetId
        );

        if (existingLink) {
          existingLink.value += count;
        } else {
          linkArray.push({ source: sourceId, target: targetId, value: count });
        }
      }
    }

    return { nodes: nodeArray, links: linkArray };
  }, [evolutionPaths]);

  const width = 800;
  const height = 400;
  const margin = { top: 20, right: 20, bottom: 20, left: 20 };

  const sankeyGenerator = sankey<CustomSankeyNode, CustomSankeyLink>()
    .nodeWidth(15)
    .nodePadding(10)
    .extent([
      [margin.left, margin.top],
      [width - margin.right, height - margin.bottom],
    ]);

  const { nodes: sankeyNodes, links: sankeyLinks } = sankeyGenerator({
    nodes: nodes.map((d) => ({ ...d })) as CustomSankeyNode[],
    links: links.map((d) => ({ ...d })) as CustomSankeyLink[],
  });

  const stageLabels = ["Pre-flop", "Flop", "Turn", "River"];

  return (
    <Card>
      <CardHeader>
        <CardTitle>Hand Evolution: {handName}</CardTitle>
      </CardHeader>
      <CardContent>
        <div className="overflow-x-auto">
          <svg width={width} height={height} className="mx-auto">
            {/* Stage labels */}
            {stageLabels.map((label, i) => (
              <text
                key={i}
                x={margin.left + (i * (width - margin.left - margin.right)) / 3}
                y={margin.top - 10}
                className="text-xs fill-muted-foreground text-center"
                textAnchor="middle"
              >
                {label}
              </text>
            ))}

            {/* Links */}
            <g className="links">
              {sankeyLinks.map((link, i) => (
                <path
                  key={i}
                  d={sankeyLinkHorizontal()(link) || ""}
                  fill="none"
                  stroke="#8884d8"
                  strokeWidth={Math.max(1, link.width || 0)}
                  opacity={0.3}
                  className="hover:opacity-60 transition-opacity"
                >
                  <title>{`${(link.source as CustomSankeyNode).name} → ${(link.target as CustomSankeyNode).name}: ${link.value}`}</title>
                </path>
              ))}
            </g>

            {/* Nodes */}
            <g className="nodes">
              {sankeyNodes.map((node, i) => (
                <g key={i}>
                  <rect
                    x={node.x0}
                    y={node.y0}
                    width={(node.x1 || 0) - (node.x0 || 0)}
                    height={(node.y1 || 0) - (node.y0 || 0)}
                    fill="#69b3a2"
                    stroke="#000"
                    strokeWidth={1}
                    className="cursor-pointer hover:fill-teal-600 transition-colors"
                  >
                    <title>{`${node.name}: ${node.value}`}</title>
                  </rect>
                  <text
                    x={node.x0! < width / 2 ? node.x1! + 6 : node.x0! - 6}
                    y={(node.y0! + node.y1!) / 2}
                    dy="0.35em"
                    textAnchor={node.x0! < width / 2 ? "start" : "end"}
                    className="text-xs fill-foreground pointer-events-none"
                  >
                    {node.name}
                  </text>
                </g>
              ))}
            </g>
          </svg>
        </div>
        <div className="mt-4 text-xs text-muted-foreground text-center">
          Flow shows how your hand strength evolves from Pre-flop to River across all simulations.
        </div>
      </CardContent>
    </Card>
  );
};

import { DashboardClient } from "./_components/dashboard-client";

// nodeId hardcoded for now — will come from user session / device selection once multi-node UI lands
export default function DashboardPage() {
  return <DashboardClient nodeId="node1" />;
}

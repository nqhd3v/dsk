"use client"

import * as React from "react"
import { AppHeader } from "@/components/organisms/app-header"
import { DeviceList, type DeviceEntry } from "@/components/organisms/device-list"
import { DeviceDetail, type DeviceDetailData } from "@/components/organisms/device-detail"

const MOCK_DEVICES: DeviceEntry[] = [
  {
    nodeId: "node1",
    name: "My Desk",
    status: "active",
    ip: "10.10.10.2",
    firmware: "0.2.0",
    lastSeenLabel: "2s ago",
    temperature: 26.3,
    present: true,
  },
  {
    nodeId: "node2",
    name: "Meeting Room",
    status: "offline",
    ip: "10.10.10.3",
    firmware: "0.2.0",
    lastSeenLabel: "3h ago",
  },
  {
    nodeId: "node3",
    name: "node3",
    status: "pending",
    ip: "10.10.10.4",
    firmware: "0.2.0",
    lastSeenLabel: "10s ago",
  },
]

const MOCK_DETAIL: DeviceDetailData = {
  nodeId: "node1",
  name: "My Desk",
  status: "active",
  ip: "10.10.10.2",
  mac: "AA:BB:CC:DD:EE:FF",
  firmware: "0.2.0",
  lastSeenLabel: "2 seconds ago",
  registeredLabel: "May 29, 2026",
  latestReading: {
    temperature: 26.3,
    humidity: 62,
    co2: 1024,
    lux: 342,
    vocKOhm: 48,
    pressure: 1013,
  },
}

export default function DevicesPage() {
  const [selected, setSelected] = React.useState<string | null>(null)

  const detail = selected === "node1" ? MOCK_DETAIL : null

  return (
    <div className="flex flex-col min-h-screen">
      <AppHeader nodeId="node1" online />

      <main className="flex-1 p-5 max-w-3xl mx-auto w-full flex flex-col gap-5">
        <DeviceList
          devices={MOCK_DEVICES}
          total={3}
          onSelect={(id) => setSelected(id === selected ? null : id)}
          onApprove={(id) => console.log("approve", id)}
          onReject={(id) => console.log("reject", id)}
        />

        {detail && (
          <DeviceDetail
            device={detail}
            onBack={() => setSelected(null)}
            onThresholds={() => {}}
            onDashboard={() => {}}
            onRestart={() => {}}
            onRemove={() => {}}
            onRename={(name) => console.log("rename", name)}
          />
        )}
      </main>
    </div>
  )
}

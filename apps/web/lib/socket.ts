"use client";

import { EnvTelemetry } from "@dsk/schemas";
import { io, Socket } from "socket.io-client";

interface IServerToClientEvents {
  telemetry: (payload: { nodeId: string; data: EnvTelemetry }) => void;
}

// eslint-disable-next-line @typescript-eslint/no-empty-object-type
export type TClientToServerEvents = {};

export type TAppSocket = Socket<IServerToClientEvents, TClientToServerEvents>;

let socket: TAppSocket | null = null;

export function getSocket(): TAppSocket {
  if (socket) {
    return socket;
  }

  const url = process.env.NEXT_PUBLIC_SOCKET_URL ?? "ws://localhost:4000";

  socket = io(url + "/telemetry", {
    transports: ["websocket"],
    autoConnect: false,
    reconnection: true,
    reconnectionAttempts: Infinity,
    reconnectionDelay: 2000,
    reconnectionDelayMax: 5000,
    timeout: 20000,
  });

  /**
   * Debug
   */
  socket.on("connect", () => {
    console.log("[socket] connected", socket?.id);
  });

  socket.on("disconnect", (reason) => {
    console.log("[socket] disconnected", reason);
  });

  socket.io.on("reconnect", (attempt) => {
    console.log("[socket] reconnected", attempt);
  });

  socket.io.on("reconnect_attempt", (attempt) => {
    console.log("[socket] reconnect attempt", attempt);
  });

  socket.io.on("error", (err) => {
    console.error("[socket] manager error", err);
  });

  return socket;
}

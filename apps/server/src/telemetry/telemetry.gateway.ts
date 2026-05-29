import { WebSocketGateway, WebSocketServer } from '@nestjs/websockets';
import { Logger } from '@nestjs/common';
import { Server } from 'socket.io';
import type { EnvTelemetry } from '@dsk/schemas';

interface ServerToClientEvents {
  telemetry: (payload: { nodeId: string; data: EnvTelemetry }) => void;
}

@WebSocketGateway({
  cors: { origin: '*' },
  namespace: '/telemetry',
})
export class TelemetryGateway {
  private readonly logger = new Logger(TelemetryGateway.name);

  @WebSocketServer()
  server!: Server<Record<string, never>, ServerToClientEvents>;

  // eslint-disable-next-line @typescript-eslint/no-unsafe-call
  broadcastTelemetry(nodeId: string, data: EnvTelemetry): void {
    // eslint-disable-next-line @typescript-eslint/no-unsafe-call, @typescript-eslint/no-unsafe-member-access
    this.server.emit('telemetry', { nodeId, data });
    // eslint-disable-next-line @typescript-eslint/no-unsafe-call, @typescript-eslint/no-unsafe-member-access
    this.server.to(nodeId).emit('telemetry', { nodeId, data });
    this.logger.debug(`WS broadcast: ${nodeId}`);
  }
}

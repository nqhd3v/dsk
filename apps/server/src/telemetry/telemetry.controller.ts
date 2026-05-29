import { Controller, Get, Param, Query } from '@nestjs/common';
import { TelemetryService } from './telemetry.service.js';

@Controller('api/telemetry')
export class TelemetryController {
  constructor(private readonly telemetryService: TelemetryService) {}

  @Get(':nodeId/recent')
  async getRecent(
    @Param('nodeId') nodeId: string,
    @Query('limit') limit?: string,
  ) {
    const n = Math.min(parseInt(limit ?? '100', 10) || 100, 1000);
    return this.telemetryService.getRecent(nodeId, n);
  }

  @Get(':nodeId/latest')
  async getLatest(@Param('nodeId') nodeId: string) {
    return this.telemetryService.getLatest(nodeId);
  }
}

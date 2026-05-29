import {
  Entity,
  PrimaryGeneratedColumn,
  Column,
  CreateDateColumn,
  UpdateDateColumn,
} from 'typeorm';

@Entity('devices')
export class DeviceEntity {
  @PrimaryGeneratedColumn('uuid')
  id!: string;

  @Column({ type: 'varchar', length: 64, unique: true })
  node_id!: string;

  @Column({ type: 'varchar', length: 64, default: '' })
  name!: string;

  @Column({
    type: 'varchar',
    length: 16,
    default: 'pending',
  })
  status!: 'pending' | 'active' | 'offline' | 'removed';

  @Column({ type: 'varchar', length: 32, nullable: true })
  fw_version!: string | null;

  @Column({ type: 'varchar', length: 17, nullable: true })
  mac!: string | null;

  @Column({ type: 'varchar', length: 45, nullable: true })
  ip!: string | null;

  @Column({ type: 'timestamptz', nullable: true })
  last_seen_at!: Date | null;

  @CreateDateColumn({ type: 'timestamptz' })
  created_at!: Date;

  @UpdateDateColumn({ type: 'timestamptz' })
  updated_at!: Date;
}

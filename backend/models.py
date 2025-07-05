from sqlalchemy import (
    create_engine, Column, Integer, Float, Boolean, DateTime
)
from sqlalchemy.orm import declarative_base, sessionmaker
from datetime import datetime
import os
DB_URL = os.getenv("DATABASE_URL", "sqlite:///shots.db")
engine = create_engine(DB_URL, echo=False, future=True)
Session = sessionmaker(bind=engine)
Base = declarative_base()


class Shot(Base):
    __tablename__ = "shots"
    id         = Column(Integer, primary_key=True)
    ts_release = Column(Float)    # ms timestamp
    ts_apex    = Column(Float)
    classification = Column(Integer)   # 0 early, 1 perfect, 2 late
    scored     = Column(Boolean)

    grip_peak  = Column(Integer)
    jump_height = Column(Float)

    created_at = Column(DateTime, default=datetime.utcnow)


def init_db():
    Base.metadata.create_all(engine)

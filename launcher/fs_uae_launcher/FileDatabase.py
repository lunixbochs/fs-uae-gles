from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

import os
import sqlite3
from .Settings import Settings

class FileDatabase:
    
    instance = None
    
    @classmethod
    def get_database_path(self):
        path = Settings.get_launcher_dir()
        path = os.path.join(path, "Files.sqlite")
        return path

    @classmethod
    def get_instance(cls):
        if not cls.instance:
            cls.instance = FileDatabase()
        return cls.instance

    def __init__(self):
        self.connection = None
        self.cursor = None

    def init(self):
        if self.connection:
            return
        self.connection = sqlite3.connect(self.get_database_path())
        self.cursor = self.connection.cursor()
        try:
            self.cursor.execute("SELECT count(*) FROM file")
        except sqlite3.OperationalError:
            self.cursor.execute("CREATE TABLE file (path string, sha1 string)")
            self.cursor.execute("CREATE INDEX file_sha1 ON file(sha1)")

    def find_file(self, sha1=""):
        self.init()
        self.cursor.execute("SELECT path FROM file WHERE sha1 = ? LIMIT 1",
                (sha1,))
        row = self.cursor.fetchone()
        if row:
            return row[0]
        else:
            return None

    def add_file(self, path, sha1):
        self.init()
        self.cursor.execute("INSERT INTO file (path, sha1) VALUES (?, ?)",
                (path, sha1))

    def commit(self, sha1=""):
        self.init()
        self.connection.commit()

    def clear(self):
        self.init()
        self.cursor.execute("DELETE FROM file")

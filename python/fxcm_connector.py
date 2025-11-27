import time
from forexconnect import fxcorepy, ForexConnect

class FXCMCLient:
    def __init__(self, username, password, connection, url):
        self.username = username
        self.password = password
        self.connection = connection # "Demo" or "Real"
        self.url = url
        self.client = ForexConnect()
        self.session = None
    
    def connect(self):
        print("Connecting to FXCM...")
        try:
            self.client.login(self.username, self.password, self.url, self.connection)
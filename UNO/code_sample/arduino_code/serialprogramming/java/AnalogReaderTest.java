/***
 * Excerpted from "Arduino: A Quick-Start Guide",
 * published by The Pragmatic Bookshelf.
 * Copyrights apply to this code. It may not be used to create training material, 
 * courses, books, articles, and the like. Contact us if you are in doubt.
 * We make no guarantees that this code is fit for any purpose. 
 * Visit http://www.pragmaticprogrammer.com/titles/msard for more book information.
***/
import java.io.InputStream;
import java.io.OutputStream;
import gnu.io.CommPortIdentifier; 
import gnu.io.SerialPort;

class AnalogReader {
  private InputStream  _input;
  private OutputStream _output;

  public AnalogReader(
    final String portName, 
    final int    baudRate) throws Exception
  {
    final int timeout = 1000;
    final String appName = "analog reader client";
    CommPortIdentifier portId = 
      CommPortIdentifier.getPortIdentifier(portName);
    SerialPort port = (SerialPort)portId.open(
      appName,
      timeout
    );
    _input = port.getInputStream();
    _output = port.getOutputStream();
    port.setSerialPortParams(
      baudRate,
      SerialPort.DATABITS_8,
      SerialPort.STOPBITS_1,
      SerialPort.PARITY_NONE
    );
  }

  public void run() throws Exception {
    byte[] buffer = new byte[255];
    Thread.sleep(2000);
    while (true) {
      _output.write("a0\n".getBytes());
      Thread.sleep(100);
      if (_input.available() > 0) {
        _input.read(buffer);
        System.out.print(new String(buffer));
      }
    }
  }
}

public class AnalogReaderTest {
  public static void main(String[] args) throws Exception {
    if (args.length != 1) {
      System.out.println(
        "You have to pass the name of a serial port."
      );
      System.exit(1);
    }
    AnalogReader analogReader = new AnalogReader(
      args[0], 9600
    );
    analogReader.run();
  }
}

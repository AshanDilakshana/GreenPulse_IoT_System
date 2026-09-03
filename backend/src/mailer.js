const nodemailer = require('nodemailer');

const sendAlertEmail = async (subject, text) => {
  if (!process.env.GMAIL_USER || !process.env.GMAIL_APP_PASSWORD || !process.env.ALERT_EMAIL_RECIPIENT) {
    console.log("Email credentials not fully configured. Skipping email alert.");
    return;
  }

  try {
    const transporter = nodemailer.createTransport({
      service: 'gmail',
      auth: {
        user: process.env.GMAIL_USER,
        pass: process.env.GMAIL_APP_PASSWORD
      }
    });

    const mailOptions = {
      from: process.env.GMAIL_USER,
      to: process.env.ALERT_EMAIL_RECIPIENT,
      subject: subject,
      text: text
    };

    const info = await transporter.sendMail(mailOptions);
    console.log('Alert email sent: ' + info.response);
  } catch (error) {
    console.error('Error sending email:', error);
  }
};

module.exports = { sendAlertEmail };
